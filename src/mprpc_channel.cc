#include "mprpc_channel.h"
#include "rpc_header.pb.h"
#include "mprpc_application.h"
#include "logger.h"
#include "zookeeper_util.h"

#include <google/protobuf/descriptor.h>     // 对应描述的功能
#include <google/protobuf/message.h>
#include <sys/types.h>                      // socket       
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>                         // close

#include <errno.h>

/*
    函数功能：
        将请求字符流序列化、发送给rpc服务端、然后阻塞等待rpc服务端返回的响应结果并反序列化
        注意：该函数属于rpc框架部分

    字符流：header_size（二进制形式） + service_name method_name args_size + args_str
        header_size（二进制形式）: header的长度
        header: service_name method_name + args_size
        args_str
*/
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done)
{
    // header: service_name method_name + args_size
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    int args_size = 0;          // 获取参数的序列化字符串长度
    std::string args_str;
    if( request->SerializeToString(&args_str) ){
        args_size = args_str.size();
    }else{
        LOG_ERROR("%s:%s:%d => serialize request error!", __FILE__, __FUNCTION__, __LINE__);
        return;
    }

    // 序列化header，同时获取header_size
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if( rpcHeader.SerializeToString(&rpc_header_str) ){
        header_size = rpc_header_str.size();
    }else{
        LOG_ERROR("%s:%s:%d => serialize request error!", __FILE__, __FUNCTION__, __LINE__);
        return;
    }

    // 1. 组织待发送的rpc请求字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    // 打印调试信息
    printf("===========================================\n");
    printf("header_size:%d\n", header_size);
    printf("rpc_header_str:%s\n", rpc_header_str.c_str());
    printf("service_name:%s\n", service_name.c_str());
    printf("method_name:%s\n", method_name.c_str());
    printf("args_str:%s\n", args_str.c_str());
    printf("===========================================\n");

    // 2. 使用tcp编程，完成rpc方法的远程调用
    // 默认创建的是阻塞型套接字（send和recv时，如果数据不可用，则阻塞等待）
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM表示创建的是一个面向连接的、可靠的、基于字节流的套接字。
    
    if( -1 == clientfd ){
        LOG_ERROR("%s:%s:%d => create socket error! errno:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        char buf[512] = {0};
        sprintf(buf, "create socket error! errno:%d", errno);
        controller->SetFailed(buf);
        return;
    }

    // 读取配置文件
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserver_ip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserver_port").c_str());
    
    // rpc调用想调用的service_name的method_name服务，需要在 zookeeper 上查询该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();      // 连接zookeeper，创建三个线程（API调用线程、网络I/O线程、watcher回调线程）
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if(host_data == ""){
        controller->SetFailed(service_name + "." + method_name + ": not found service!");
        return;
    }
    int idx = host_data.find(":");
    if(idx == -1){
        controller->SetFailed(method_path + "address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - 1 - idx + 1).c_str());


    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    // 连接rpc服务节点
    if( -1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) ){
        LOG_ERROR("%s:%s:%d => connect error! errno:%d", __FILE__, __FUNCTION__, __LINE__, errno);  
        char buf[512] = {0};
        sprintf(buf, "connect error! errno:%d", errno);
        controller->SetFailed(buf);
        exit(EXIT_FAILURE);
    }

    // 发送rpc请求
    if( -1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) ){
        LOG_ERROR("%s:%s:%d => send error! errno:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        char buf[512] = {0};
        sprintf(buf, "send error! errno:%d", errno);
        controller->SetFailed(buf);
        return;
    }

    // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if( -1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)) ){
        LOG_ERROR("%s:%s:%d => recv error! errno:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        char buf[512] = {0};
        sprintf(buf, "recv error! errno:%d", errno);
        controller->SetFailed(buf);
        return;
    }

    // 反序列化rpc调用的响应数据
    // std::string response_str(recv_buf, 0, recv_size);       // bug  recv_buf遇到\0时，默认到达字符串尾
    // if( !response->ParseFromString(response_str) ){
    if(!response->ParseFromArray(recv_buf, recv_size)){
        LOG_ERROR("%s:%s:%d => parse error! response_str: %s", __FILE__, __FUNCTION__, __LINE__, recv_buf);
        char buf[512] = {0};
        sprintf(buf, "parse error! errno:%d", errno);
        controller->SetFailed(buf);
        return;
    }

    close(clientfd);
    return;
}


