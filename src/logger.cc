#include "logger.h"

#include <iostream>
#include <time.h>


// 获取日志的单例
Logger& Logger::GetInstance(){
    static Logger logger;       // 静态变量是线程安全的
    return logger;
}


// 无参构造
Logger::Logger(){
    // 启动专门的写日志线程
    std::thread writeLogTask([&](){
        for(;;){
            // 获取当天的日期，然后取日志信息写入相应的日志文件中
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char file_name[128];
            sprintf(file_name, "%d-%d-%d-log.txt", nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday);

            FILE *pf = fopen(file_name, "a+");  // 打开文件，文件不存在则创建
            if (pf == nullptr){
                printf("logger file:%s open error\n", file_name);
                exit(EXIT_FAILURE);             // 退出进程
            }
            
            std::string msg = m_lockQueue.Pop();

            char log_buf[256] = {0};

            sprintf(log_buf, "[%s]:%d:%d:%d => ", 
                m_logLevel == INFO ? "INFO" : "ERROR", nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);

            msg.insert(0, log_buf);
            msg.append("\n");

            fputs(msg.c_str(), pf);
            fclose(pf);
        }
    });
    // 设置分离线程（分离后的线程变成一个独立的执行单元，它有自己的生命周期，不再受创建它的线程控制。即使创建它的线程结束，分离的线程也会继续执行直到它自己的任务完成或被系统终止。）
    // 此外一个容易混淆的概念，守护线程（当主线程结束时，无论守护线程是否还在运行，都会被强制终止。）
    writeLogTask.detach();
}


// 设置日志级别 
void Logger::SetLogLevel(LogLevel Level){
    m_logLevel = Level;
}


// 写日志。把日志信息写入 m_lockQueue缓冲区 中
void Logger::Log(std::string msg){
    m_lockQueue.Push(msg);
}

