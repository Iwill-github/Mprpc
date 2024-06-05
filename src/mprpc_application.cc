#include "mprpc_application.h"

#include <iostream>
#include <unistd.h>
#include <string>


MprpcConfig MprpcApplication::m_config;


void ShowArgsHelp(){
    std::cout << "format: command -i <configfile>" << std::endl;
}


// 框架初始化
void MprpcApplication::Init(int argc, char **argv){
    if(argc < 2){
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    /*
        getopt()每次调用都会返回下一个命令行选项的字符，
            如果是有效选项，则返回对应字符；
            如果是非法选项或者遇到非选项参数，返回'?';；
            如果是丢失了参数，即没有提供对应的参数，返回':'
            如果没有更多选项，返回-1。
    */
    while((c = ::getopt(argc, argv, "i:")) != -1){
        switch(c){
            case 'i':
                config_file = optarg;
                break;
            case '?':
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            case ':':
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    // 开始加载配置文件 rpcserver_ip、rpcserver_port、zookeeper_ip、zookeeper_port
    m_config.LoadConfigFile(config_file.c_str());   // Init 方法为静态方法，其访问m_config，要求m_config也为静态成员

    // test
    // std::cout << "rpcserver_ip:" << m_config.Load("rpcserver_ip") << std::endl;
    // std::cout << "rpcserver_port:" << m_config.Load("rpcserver_port") << std::endl;
    // std::cout << "zookeeper_ip:" << m_config.Load("zookeeper_ip") << std::endl;
    // std::cout << "zookeeper_port:" << m_config.Load("zookeeper_port") << std::endl;
}


MprpcApplication& MprpcApplication::GetInstance(){
    static MprpcApplication app;
    return app;
}


MprpcConfig& MprpcApplication::GetConfig(){
    return m_config;
}