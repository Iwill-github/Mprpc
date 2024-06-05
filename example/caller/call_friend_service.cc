#include <iostream>

#include "mprpc_application.h"
#include "friend.pb.h"
#include "mprpc_channel.h"


int main(int argc, char** argv){
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());   // RpcChannel 可以理解为中继，不要理解为通道

    // 设置请求的参数
    fixbug::GetFriendsListRequest request;
    request.set_userid(1);

    // 创建rpc的响应对象
    fixbug::GetFriendsListResponse response;

    // 创建代理对象，帮忙做数据序列化、反序列化、数据收发
    // 使用代理对象调用方法，最终都会转到 RpcChannel->RpcChannel::callMethod 上，集中来做所有rpc方法调用的 参数序列化、网络发送
    // 注意，这是一个同步的rpc方法调用过程
    stub.GetFriendsList(nullptr, &request, &response, nullptr); 

    // 一次rpc调用完成，读取响应结果
    if( 0 == response.result().errcode() ){
        printf("rpc GetFriendsList response success! \n");
        for(int i = 0; i < response.friends_size(); ++i){
            printf("name:%s\n", response.friends(i).c_str());
        }
    }else{
        printf("rpc GetFriendsList response error:%s\n", response.result().errmsg().c_str());
    }

    return 0;
}

