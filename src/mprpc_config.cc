#include "mprpc_config.h"
#include "logger.h"
// #include "Logging.h"

#include <iostream>

void MprpcConfig::Trim(std::string& read_buf){
    // 去掉字符串前多余的空格
    int idx = read_buf.find_first_not_of(" ");  // 第一个不是空格的位置
    if(idx != -1){
        read_buf = read_buf.substr(idx, read_buf.size() - idx);    // 字符串前有空格
    }

    // 去掉字符串后面的空格
    idx = read_buf.find_last_not_of(" ");       // 最后一个不是空格的位置
    if(idx != -1){
        read_buf = read_buf.substr(0, idx + 1);     // 字符串后面有空格
    }
}


// 读取解析配置文件
void MprpcConfig::LoadConfigFile(const char *config_file){
    FILE *pf = fopen(config_file, "r");
    if(nullptr == pf){
        LOG_ERROR("%s:%s:%d => %s file error!", __FILE__, __FUNCTION__, __LINE__, config_file);
        // LOG << "config file error!";
        exit(EXIT_FAILURE);
    }

    // 处理：1.注释     2.正确的配置项 =   3.去掉开头的多余的空格
    while(!feof(pf)){   // feof(pf) 判断文件是否结束
        char buf[512];
        fgets(buf, 512, pf);    // 读取到换行符或者缓冲区512字节结束

        std::string read_buf(buf);

        // 去除字符串前后空格
        Trim(read_buf);      

        // 判断 # 的注释、空行
        if(read_buf[0] == '#' && read_buf.empty()){
            continue;
        }
 
        // 解析配置项
        int idx = read_buf.find("=");
        if(idx == -1){      // 配置项不合法
            continue;
        }

        std::string key = read_buf.substr(0, idx);
        std::string value = read_buf.substr(idx + 1, read_buf.size() - 1 - idx - 1);    // 最后一个-1是去除 \n

        Trim(key);          // 去除前后空格
        Trim(value);

        // m_configMap[key] = value;        // 已经存在key时，对应的value会被覆盖
        m_configMap.insert({key, value});   // 已经存在key时，会返回一个 std::pair<iterator, bool>
    }
}



// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key){
    // return m_configMap[key];         
    // 如果map包含key，没有问题，如果map不包含key，使用下标有一个危险的副作用，会在map中插入一个key的元素，value取默认值，返回value。也就是说，map[key]不可能返回null。
    
    if(m_configMap.count(key) == 0){
        return "";
    }

    return m_configMap[key];
}      

