#include <iostream>

#include "mprpc_application.h"
#include "user.pb.h"
#include "mprpc_channel.h"


int main(int argc, char** argv){
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());   // RpcChannel 可以理解为中继，不要理解为通道

    // 1. 演示调用远程发布的rpc方法Login    
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    fixbug::LoginResponse response;

    // 创建代理对象，帮忙做数据序列化、反序列化、数据收发
    // 使用代理对象调用方法，最终都会转到 RpcChannel->RpcChannel::callMethod 上，集中来做所有rpc方法调用的 参数序列化、网络发送
    // 注意，这是一个同步的rpc方法调用过程
    stub.Login(nullptr, &request, &response, nullptr); 

    // 一次rpc调用完成，读取响应结果
    if( 0 == response.result().errcode() ){
        printf("rpc login response success:%d\n", response.success());
    }else{
        printf("rpc login response error:%s\n", response.result().errmsg().c_str());
    }



    // 2. 演示调用远程发布的rpc方法 Register
    fixbug::RegisterRequest register_request;
    register_request.set_id(2000);
    register_request.set_name("mprpc");
    register_request.set_pwd("123456");

    fixbug::RegisterResponse register_response;
    
    stub.Register(nullptr, &register_request, &register_response, nullptr);

    // 一次rpc调用完成，读取响应结果
    if( 0 == register_response.result().errcode() ){
        printf("rpc register response success:%d\n", register_response.success());
    }else{
        printf("rpc register response error:%s\n", register_response.result().errmsg().c_str());
    }


    return 0;
}


