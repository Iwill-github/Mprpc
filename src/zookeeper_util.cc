#include "zookeeper_util.h"
#include "mprpc_application.h"
#include "logger.h"

#include <semaphore.h>
#include <semaphore.h>
#include <iostream>


/*
    问题记录：-- 20240605
        该文件代码运行在客户端中，所以启动的子线程属于客户端子线程，这可能是不能使用日志的原因？
*/


// 全局的watcher观察器      zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx){
    if (type == ZOO_SESSION_EVENT) {                    // 回调的消息类型是和会话相关的消息类型
        if (state == ZOO_CONNECTED_STATE) {             // zkclient和zkserver连接成功
            sem_t *sem = (sem_t*)zoo_get_context(zh);   // sem是zkclient中定义的信号量
            sem_post(sem);                              // 唤醒信号量
        }
    }
}



ZkClient::ZkClient(): m_zhangdle(nullptr){
    
}



ZkClient::~ZkClient(){
    if(m_zhangdle != nullptr){
        zookeeper_close(m_zhangdle);    // 关闭句柄，释放资源
    }
}



// 连接zk服务器
void ZkClient::Start(){
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeper_ip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeper_port");
    std::string connstr = host + ":" + port;

    /*
        zookeeper_mt: 多线程版本
        zookeeper 的API客户端程序提供了三个线程：
            API调用线程、
            网络I/O线程、（pthread_create poll）
            watcher回调线程（pthread_create  负责和当前线程通信）
    */
    // 创建句柄。注意：并不是句柄创建成功，就是zkclient和zkserver连接成功
    m_zhangdle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);   
    if(m_zhangdle == nullptr){  // 并不代表zkclient和zkserver连接不成功，只是代表 m_zhangdle 内存初始化失败
        // LOG_ERROR("%s:%s:%d => connect zookeeper failed!", __FILE__, __FUNCTION__, __LINE__);
        std::cout << "connect zookeeper failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhangdle, &sem);

    sem_wait(&sem);     //  Wait for SEM being posted.
    // LOG_INFO("%s:%s:%d => connect zookeeper success!", __FILE__, __FUNCTION__, __LINE__);
    std::cout << "connect zookeeper success!" << std::endl;
}   



// 在zkserver上根据path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state){
    char path_buffer[128];
    int bufflen = sizeof(path_buffer);
    int flag = zoo_exists(m_zhangdle, path, 0, nullptr);

    if(ZNONODE == flag){    // path不存在
        flag = zoo_create(m_zhangdle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufflen);
        if(flag == ZOK){
            // LOG_INFO("%s:%s:%d => znode create success... path:%s", __FILE__, __FUNCTION__, __LINE__, path);
            std::cout << "znode create success... path:" << path << std::endl;
        }else{
            // LOG_ERROR("%s:%s:%d => znode create failed... flag:%d, path:%s", __FILE__, __FUNCTION__, __LINE__, flag, path);
            std::cout << "znode create failed... flag:" << flag << " path:" << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return;
}  



// 根据参数path获取znode节点的值
std::string ZkClient::GetData(const char *path){
    char buffer[64];
    int bufferlen = sizeof(buffer);

    int flag = zoo_get(m_zhangdle, path, 0, buffer, &bufferlen, nullptr);

    if(flag != ZOK){
        // LOG_ERROR("%s:%s:%d => get znode error..., path:%s", __FILE__, __FUNCTION__, __LINE__, path);
        std::cout << "get znode error..., path:" << path << std::endl;
        return "";
    }else{
        return buffer;
    }
}       


