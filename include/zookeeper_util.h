#pragma once 

#include <zookeeper/zookeeper.h>
#include <string>


// 封装的zookeeper客户端类
class ZkClient{
public:
    ZkClient();
    ~ZkClient();

    void Start();                                                               // 连接zk服务器
    void Create(const char *path, const char *data, int datalen, int state=0);  // 在zkserver上根据path创建znode节点
    std::string GetData(const char *path);                                      // 根据参数path获取znode节点的值

private:
    // zk的客户端句柄
    zhandle_t *m_zhangdle;
};

