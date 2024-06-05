#include <iostream>
#include <string>
#include <vector>

#include "friend.pb.h"
#include "mprpc_application.h"
#include "mprpc_provider.h"


class FriendService : public fixbug::FriendServiceRpc {
public:
    std::vector<std::string> GetFriendsList(uint32_t user_id){
        printf("doing local service: GetFriendsList! userid:%d\n", user_id);

        std::vector<std::string> vec;
        vec.emplace_back("zhangsan");
        vec.emplace_back("lisi");
        vec.emplace_back("wangwu");

        return vec;
    }


    // 重写基类方法
    void GetFriendsList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // 获取请求参数
        uint32_t user_id = request->userid();

        // 做本地业务
        std::vector<std::string> friends_list = GetFriendsList(user_id);
        
        // 写入响应
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for(std::string& name : friends_list){
            std::string* p = response->add_friends();
            *p = name;
        }

        // 执行回调操作（把LoginResponse发送给 rpc client）
        if (done){
            done->Run();
        }
    }
};



int main(int argc, char** argv){
    // 调用框架初始化操作   provider -i config.conf
    MprpcApplication::Init(argc, argv);
    
    // provider 是一个 rpc 网络服务对象（必须保证高并发，负责数据序列化、反序列化和网络收发），把 UserService对象 发布到 rpc 节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动一个rpc服务发布节点。Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}



