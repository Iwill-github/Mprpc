#pragma once

#include "mprpc_config.h"

/*
    MprpcApplication 类：mprpc 框架基础类，负责框架的一些初始化操作
*/
class MprpcApplication{
public:
    static void Init(int argc, char **argv);    // 框架初始化操作
    static MprpcApplication& GetInstance();     // 获取MprpcApplication对象
    static MprpcConfig& GetConfig();

private:
    static MprpcConfig m_config;

    MprpcApplication(){};
    MprpcApplication(const MprpcApplication&) = delete;

    // 这里的&&是右值引用，它允许移动构造函数接收一个即将销毁的对象（右值）的引用，并“移动”其内部资源到新创建的对象中。
    // 移动构造函数常常用来处理智能指针或其他资源，通过交换指针或资源的拥有权，而不是进行深拷贝，从而提高效率。
    MprpcApplication(MprpcApplication&&) = delete;
};


