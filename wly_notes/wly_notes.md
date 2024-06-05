
# 常用命令
    sudo netstat -tanp | grep zookeeper
        -a：显示所有连接和监听端口，包括正在使用的和未使用的端口。
        -n：使用数字格式显示地址和端口号，而不进行反向域名解析。
        -p：显示与每个连接或监听端口相关的进程的PID和进程名称。
    ps -ef | grep zookeeper
    telnet <hostname or IP> <port>
    
    ifconfig
    tcpdump -i lo port  2181    tcpdump命令用于在网络接口上捕获和分析网络数据包。使用tcpdump抓取本地环回网卡lo上端口2181的数据报）


# 业界优秀的RPC框架：
    baidu的brpc、google的grpc


# Mprpc:
    项目基于muduo高性能网络库 + Protobuf（序列化反序列化）+ 服务注册中心 开发，所以命名为 mprcp


# protobuf安装配置
## google protobuf安装：https://github.com/protocolbuffers/protobuf
    1、解压压缩包:unzip protobuf-master
    2、进入解压后的文件夹: cd protobuf-master
    3、安装所需工具: sudo apt-get install autoconf automake libtool curl make g++ unzip     # curl 版本问题未更新成功
    4、自动生成configure配置文件:./autogen.sh
    5、配置环境:./configure
    6、编译源代码(时间比较长): make
    7、安装: sudo make install
    8、刷新动态库:sudo ldconfig


## protobuf的 proto配置文件、protoc编译命令
    1. 在test/protobuf文件下，编写test.proto配置文件
    2. 生成相应的c++代码：protoc test.proto --cpp_out=./


# gdb调试
    gdb ./provider          # 启动的是provider程序，但是想调试的代码，在so库中
    b mprpc_config.cc:52
    run -i test.conf
    n
    s
    p       查看变量信息
    
    info b                  -- 查看当前断点信息
    info threads            -- 查看当前所有的线程信息
    thread 4                -- 切换到线程4


# zookeeper分布式协调服务
## Zookeeper是什么？
    Zookeeper是一个为分布式应用提供 一致性协调服务 的中间件。
    Zookeeper是在分布式环境中应用非常广泛，优秀功能很多，比如：分布式环境中全局命名服务，服务注册中心，全局分布式锁。

## zookeeper的服务配置中心功能？
    ZooKeeper 作为服务配置中心，其核心用途是为分布式系统提供一种集中管理和分发配置信息的方式。
    具体来说，当你在 ZooKeeper 上更新了一个配置项，所有连接到 ZooKeeper 并且订阅了这个配置项的服务实例会立即收到通知，可以即时地获取并应用新的配置，无需手动干预每个服务的配置文件。这种方式极大地简化了大型分布式系统中配置的维护工作，提高了运维效率，并减少了因配置错误导致的服务故障。

## zookeeper安装：
    sudo apt install openjdk-17-jdk             -- zookeeper是java开发，依赖jdk

    tar -zxvf zookeeper-3.4.10
    cd conf
    cp zoo_sample.cfg zoo.cfg
    vim zoo.cfg                                 -- 修改 dataDir（默认使用的/tmp，重启时数据会清空）
    
    cd ../bin
    ./zkServer.sh start
    ps -ef | grep zookeeper
    sudo netstat -tanp                          -- 查看是否启动了运行在端口2181的java服务
    ./zkCli.sh

## zookeeper的znode节点，及客户端常用命令：
    临时性节点：rpc节点超时未发送心跳消息，zk会自动删除临时性节点
    永久性节点：rpc节点超时未发送心跳消息，zk不会删除这个节点

    ls、get、create、set、delete

    ls /

    get /zookeeper
                                                -- 存储数据的字段（该数据字段为空）
        cZxid = 0x0
        ctime = Thu Jan 01 08:00:00 CST 1970
        mZxid = 0x0
        mtime = Thu Jan 01 08:00:00 CST 1970
        pZxid = 0x0
        cversion = -1
        dataVersion = 0
        aclVersion = 0
        ephemeralOwner = 0x0                    -- 永久/临时节点字段
        dataLength = 0
        numChildren = 1
    
    create /mprpc 20                            -- 创建永久节点
    set /mprpc 30
    delete /mprpc


## zookeeper的watcher机制
    Zookeeper的Watcher机制是一种事件触发模型，它允许客户端注册监听Zookeeper中特定节点（znode）的变化，如创建、删除、数据更新或权限变更。当这些事件发生时，Zookeeper服务器会向注册了Watcher的客户端发送通知。
    注意：Watcher是一次性触发的，即一旦触发，除非客户端重新注册，否则不会再次触发。
    
    
## zookeeper的 原生API安装（C/C++ 接口）
    ~/package/zookeeper-3.5.7/src/c/  $ sudo ./configure
    ~/package/zookeeper-3.5.7/src/c/  $ sudo make               -- 根据Makefile文件编译
    ~/package/zookeeper-3.5.7/src/c/  $ sudo make install       -- 将编译完成的文件拷贝至 /usr/local/include、lib、bin
    ~/package/zookeeper-3.5.7/src/c/  $ sudo ldconfig           -- 刷新动态库
    注意：编译时，遇见了两个字符串拼接相关的错误，自己解决。

    原生 ZkClient API存在的问题：
        1. 设置监听watcher只能是一次性的，每次出发后需要重复设置
        2. znode节点只存储简单的byte字节数组，如果存储对象，需要自己转换对象生成字节数组





