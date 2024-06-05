#include <iostream>
#include <string>

#include "test.pb.h"

using namespace fixbug;

/*
编译命令：
    g++ main.cc test.pb.cc -lprotobuf

其他笔记：
    1. 建议将string类型写为bytes类型，减少protobuf的类型转换
    2. 序列化、反序列化   SerializeToString、 ParseFromString
    3. 类型嵌套相关       mutable_result
    4. 列表相关           add_friend_list、add_friend  
*/

void test01();
void test02();

int main(int argc, char** argv){
    test01();
    // test02();

    return 0;
}


/*
    设置子对象属性、列表相关
*/
void test02(){
    // LoginResponse rsp;
    // ResultCode *rc = rsp.mutable_result();
    // rc->set_errcode(1);
    // rc->set_errmsg("登录处理失败");

    GetFriendListsResponse rsp;
    ResultCode *rc = rsp.mutable_result();      // 获取对象的指针
    rc->set_errcode(0);

    User* user1 = rsp.add_friend_list();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(User::MAN);

    User* user2 = rsp.add_friend_list();        // 在列表中创建对象，并返回对象指针
    user2->set_name("li si");
    user2->set_age(20);
    user2->set_sex(User::MAN);

    std::cout << rsp.friend_list_size() << std::endl;   // 2
    for(int i = 0; i < rsp.friend_list_size(); i++){
        std::cout << rsp.friend_list(i).name() << std::endl;
    }
}


/*
    序列化反序列化相关
*/
void test01(){
    // 封装login请求数据
    LoginRequest req;               
    req.set_name("zhangsan");
    req.set_pwd("123456");

    // 对象数据序列化 => char*
    std::string send_str;           
    if(req.SerializeToString(&send_str)){
        std::cout << send_str << std::endl;         // zhangsan123456
    }

    // 从send_str反序列化一个login请求对象
    LoginRequest req_decode;
    if(req_decode.ParseFromString(send_str)){
        std::cout << req_decode.name() << std::endl;
        std::cout << req_decode.pwd() << std::endl;
    }
}