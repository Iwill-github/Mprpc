#pragma once

#include <string>
#include <unordered_map>


/*
    MprpcConfig: 框架读取配置文件类
        负责读取、封装配置文件内容（rpcserver_ip, rpcserver_port, zookeeper_ip, zookeeper_port）
*/
class MprpcConfig
{
public:
    MprpcConfig(){};

    void LoadConfigFile(const char *config_file);   // 读取解析配置文件
    std::string Load(const std::string &key);       // 查询配置项信息

private:
    std::unordered_map<std::string, std::string> m_configMap;

    void Trim(std::string& src_buf);                 // 去除字符串前后的空格
};

