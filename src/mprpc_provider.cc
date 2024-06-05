#include "mprpc_provider.h"
#include "mprpc_application.h"
#include "rpc_header.pb.h"
#include "logger.h"
#include "zookeeper_util.h"

#include <functional>


/*
    把 service对象 发布到 rpc 节点上，即填写 m_serviceInfoMap
        service* 记录服务对象
            pserviceDesc* 记录服务对象描述
                service_name 服务对象的名字
                methodCnt    服务对象的方法数量
                method(i)    服务对象的方法描述
*/
void RpcProvider::NotifyService(google::protobuf::Service *service){

    ServiceInfo sevice_info;
    
    // 获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务对象service的方法数量
    int methodCnt = pserviceDesc->method_count();
    // 获取服务对象的名字
    std::string service_name = pserviceDesc->name();

    LOG_INFO("%s:%s:%d => service_name:%s", 
        __FILE__, __FUNCTION__, __LINE__, service_name.c_str())

    for(int i = 0; i < methodCnt; ++i){
        // 获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        sevice_info.m_methodMap.insert({method_name, pmethodDesc});

        LOG_INFO("%s:%s:%d => method_name:%s", 
            __FILE__, __FUNCTION__, __LINE__, method_name.c_str())
    }

    sevice_info.m_service = service;
    m_serviceInfoMap.insert({service_name, sevice_info});
}



// 启动一个rpc服务节点，开始提供rpc服务。 remote procedure call (RPC)
void RpcProvider::Run(){
    
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserver_ip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserver_port").c_str());
    muduo::net::InetAddress address(ip, port);

    // 创建 TCPServer 对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    // 绑定 连接回调 和 消息读写回调
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));  // 传入的函数应该只有1个参数，然而OnConnection函数是两个参数（this参数被隐藏了），故需要适配器将其适配为单参数函数
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));  // 将OnMessage适配为3参数函数（OnMessage本是4参数函数）

    // 设置 muduo库的线程数量
    server.setThreadNum(4);


    // 把当前rpc节点上要发布的服务全部注册到zk上面，让 rpc client可以从 zk 上发现服务
    // session timeout 30s          zkClient 网络I/O线程每 1/3 * timeouot时间(10s)，会向zkServer发送ping心跳消息
    ZkClient zkCli;
    zkCli.Start();
    // service_name 为永久性节点  method_name 为临时性节点
    for(auto &service_pair : m_serviceInfoMap){
        // /service_name
        std::string service_path = "/" + service_pair.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);

        for(auto &mp: service_pair.second.m_methodMap){
            // /service_name/method_name       如：/UserServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data_buffer[128] = {0};
            sprintf(method_path_data_buffer, "%s:%d", ip.c_str(), port);
            zkCli.Create(method_path.c_str(), method_path_data_buffer, strlen(method_path_data_buffer), ZOO_EPHEMERAL);
        }
    }
    

    LOG_INFO("%s:%s:%d => RpcProvider start service at ip:%s port:%d", 
        __FILE__, __FUNCTION__, __LINE__, ip.c_str(), port)
    printf("RpcProvider start service at ip:%s port:%d", ip.c_str(), port);

    // 启动网络服务
    server.start();         // 初始化线程池（subloop）、开启监听客户端连接等
    m_eventLoop.loop();     // 启动mainloop   阻塞 epoll_wait
}



// 新socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn){
    if(!conn->connected()){
        conn->shutdown();   // 和 rpc client 连接断开
    }
}



/*
函数说明：
    已建立连接用户的读写事件回调。如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
    即，反序列化网络传输的字符流，然后抽象调用对应的 rpc服务方法（需要传入负责序列化、网络发送的回调）

其他笔记：
    在框架内部，RpcProvider和RpcConsumer需要协商好之间通信用的数据类型
    定义proto的message类型，进行数据头的序列化和反序列化
        service_name  method_name  args_size     
    数据流举例：
        16UserServiceLoginzhang san
    数据流解析：
        header_size（4个字节(二进制的形式)，服务名、方法名、参数长度(字符串形式)所占长度） 
        + header_str（service_name + method_name + args_size） 
        + args_str
*/
void RpcProvider::OnMessage(
    const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp time)
{
    // 1. 解析网路传输的字符流
    // 网络上接收的远程rpc调用请求的字符流      方法名、方法参数
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中，读取前四个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);   // 从recv_buf中，第0个位置开始拷贝4个字节的内容到header_size中

    // 根据header_size读取原始的字符流，利用protobuf反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;

    std::string service_name;
    std::string method_name;
    uint32_t args_size;

    if( rpcHeader.ParseFromString(rpc_header_str) ){    // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }else{                                              // 数据头反序列化失败
        LOG_ERROR("%s:%s:%d => rpc_header_str:%s parse errno!", __FILE__, __FUNCTION__, __LINE__, rpc_header_str.c_str());
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 打印调试信息
    printf("===========================================\n");
    printf("header_size:%d\n", header_size);
    printf("rpc_header_str:%s\n", rpc_header_str.c_str());
    printf("service_name:%s\n", service_name.c_str());
    printf("method_name:%s\n", method_name.c_str());
    printf("args_str:%s\n", args_str.c_str());
    printf("===========================================\n");

    // 2. 根据反序列化成功的数据，动态调用其指定的rpc服务方法
    // 获取service对象和method对象
    auto it = m_serviceInfoMap.find(service_name);
    if( it == m_serviceInfoMap.end() ){
        LOG_ERROR("%s:%s:%d => %s is not exist!", __FILE__, __FUNCTION__, __LINE__, service_name.c_str())
        return;
    }
    auto mit = it->second.m_methodMap.find(method_name);
    if( mit == it->second.m_methodMap.end() ){
        LOG_ERROR("%s:%s:%d => %s:%s is not exist!", __FILE__, __FUNCTION__, __LINE__, service_name.c_str(), method_name.c_str())
        return;
    }

    google::protobuf::Service *service = it->second.m_service;          // service对象  new UserService
    const google::protobuf::MethodDescriptor *method = mit->second;     // method对象   Login

    // 生成 rpc方法 要使用的 request 和 response 参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if( !request->ParseFromString(args_str) ){
        LOG_INFO("%s:%s:%d => request parse error! content: %s", __FILE__, __FUNCTION__, __LINE__, args_str.c_str());
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure* done = 
        google::protobuf::NewCallback<RpcProvider, const muduo::net::TcpConnectionPtr&, google::protobuf::Message*> 
            (this, &RpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法，并绑定 用于序列化rpc的响应、网络发送的回调函数
    // new Service().Login(method, constroller, request, response, done);
    // 注意：该处不是调用的 MprpcChannel::CallMethod 方法，而是调用的是当前rpc节点上发布的方法
    service->CallMethod(method, nullptr, request, response, done);
}



// Closure 的回调操作，用于序列化rpc的响应、网络发送
void RpcProvider::SendRpcResponse(
    const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response)  // response是序列化之前的
{
    std::string response_str;
    if( response->SerializeToString(&response_str) ){
        // 序列化成功后，通过网络把rpc方法执行的结果发送回rpc调用方（caller）
        conn->send(response_str);
    }else{
        LOG_ERROR("%s:%s:%d => serialize response_str error!", 
            __FILE__, __FUNCTION__, __LINE__);
    }

    conn->shutdown();   // 模拟tcp的短连接服务，由rpc_provider主动断开连接
}


