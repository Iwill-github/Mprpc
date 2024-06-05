#pragma once

#include "google/protobuf/service.h"

#include <memory>                   // unique_ptr
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>

#include <string>
#include <functional>   // bind
#include <google/protobuf/descriptor.h>     // pserviceDesc相关方法
#include <unordered_map>

/*
    RpcProvider类：框架提供的专门服务发布、启动rpc服务的网络对象类
        网络功能，使用muduo库实现
        
    关键成员：
        m_eventLoop
        m_serviceInfoMap <std::string, ServiceInfo>
*/ 
class RpcProvider
{
public:
    // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);         // 把 service对象 发布到 rpc 节点上
    void Run();                                                     // 启动一个rpc服务节点，开始提供rpc服务。 remote procedure call (RPC)

private:
    muduo::net::EventLoop m_eventLoop;                              // 组合EventLoop

    struct ServiceInfo{                                             // 保存服务对象和其方法描述
        google::protobuf::Service *m_service;                       // 保存服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap;    // 保存服务方法
    };
    std::unordered_map<std::string, ServiceInfo> m_serviceInfoMap;  // 保存服务对象的所有信息
    
    void OnConnection(const muduo::net::TcpConnectionPtr& conn);                                       // 新连接回调
    void OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp time);     // 读写消息回调

    // Closure 的回调操作，用于序列化rpc的响应、网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};


