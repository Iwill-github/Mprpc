#include <iostream>
#include <string>

#include "user.pb.h"
#include "mprpc_application.h"
#include "mprpc_provider.h"


/*
    UserService 原来是一个本地服务，提供了两个进程内的本地方法，Login、GetFriendLists
    本地服务如何发布为rpc服务
*/
class UserService: public fixbug::UserServiceRpc{       // 使用在rpc服务发布端（rpc提供者）
public:
    bool Login(std::string name, std::string pwd){
        printf("doing local service: Login\n");
        printf("name:%s pwd:%s\n", name.c_str(), pwd.c_str());
        return true;
    }


    bool Register(uint32_t id, std::string name, std::string pwd){
        printf("doing local service: Register\n");
        printf("id:%d name:%s pwd:%s\n", id, name.c_str(), pwd.c_str());
        return true;
    }


    /*
        重写基类 UserServiceRpc的虚函数，下面这些方法都是框架直接调用的
        此处不属于框架的代码，属于 Server的内容（call -> work -> return），参考：/wly_notes/02_RPC原理.png 的最右侧红色框。
        1. caller  ===>  Login(LoginRequest)  =>  muduo  =>  callee
        2. callee  ===>  根据接收到的 Login(LoginRequest)，调用下面重写的 Login 方法 
    */
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // rpc框架给业务上报了请求参数 LoginRequest（即，rpc框架实现了 序列化、muduo、反序列化），应用获取相应的数据做本地业务。
        std::string name = request->name(); // request 为 protobuf 中的消息类型（未序列化）
        std::string pwd = request->pwd();

        // 做本地业务
        bool login_result = Login(name, pwd);   

        // 把响应写入（包括错误码、错误消息、返回值）
        fixbug::ResultCode* code = response->mutable_result();      // 通过指针写入嵌套类
        code->set_errcode(0);
        code->set_errmsg("");
        // code->set_errcode(1);
        // code->set_errmsg("Login do error!");
        response->set_success(login_result);

        // 执行回调操作（把LoginResponse发送给 rpc client）
        if (done != nullptr) {
            done->Run();            // 执行 响应对象数据的 序列化、网络发送、反序列化（rpc框架完成的）
        }
    }


    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool register_result = Register(id, name, pwd);

        // 写入response
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(register_result);

        if (done != nullptr) {
            done->Run();            // 执行 响应对象数据的 序列化、网络发送、反序列化（rpc框架完成的）
        }
    }
};


/*
    MprpcApplication    -- mprpc 框架基础类，负责框架的一些初始化操作
    RpcProvider         -- 框架提供的专门服务发布、启动rpc服务的网络对象类
*/
int main(int argc, char** argv){
    // 调用框架初始化操作   provider -i config.conf
    MprpcApplication::Init(argc, argv);
    
    // provider 是一个 rpc 网络服务对象（必须保证高并发，负责数据序列化、反序列化和网络收发），把 UserService对象 发布到 rpc 节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点。Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}


