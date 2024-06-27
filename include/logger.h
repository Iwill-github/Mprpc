#pragma once

#include "lock_queue.h"

#include <string>
#include <memory>
#include <mutex>
#include <thread>

enum LogLevel{
    INFO,   // 普通信息
    ERROR,  // 错误信息
};

// Mprpc 提供的日志系统
class Logger{
public:
    static Logger& GetInstance();           // 获取日志的单例

    void SetLogLevel(LogLevel Level);       // 设置日志级别        
    void Log(std::string msg);              // 写日志

private:
    Logger();
    Logger(const Logger&) = delete;         // 防止使用拷贝构造、移动构造生成新对象
    Logger(Logger&&) = delete;


    int m_logLevel;                         // 记录日志级别
    LockQueue<std::string> m_lockQueue;     // 日志缓冲队列
};


// 定义宏
#define LOG_INFO(logmsgformat, ...)                     \
    do{                                                 \
        Logger &logger = Logger::GetInstance();         \
        logger.SetLogLevel(INFO);                       \
        char c[2048] = {0};                             \
        snprintf(c, 2048, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c);                                  \
    }while(0);


#define LOG_ERROR(logmsgformat, ...)                    \
    do{                                                 \
        Logger &logger = Logger::GetInstance();         \
        logger.SetLogLevel(ERROR);                      \
        char c[2048] = {0};                             \
        snprintf(c, 2048, logmsgformat, ##__VA_ARGS__); \
        logger.Log(c);                                  \
    }while(0);

