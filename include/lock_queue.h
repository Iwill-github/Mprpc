#pragma once

#include <queue>
// #include <thread>                   // pthread_t
#include <mutex>                    // pthread_mutex_t
#include <condition_variable>       // pthread_condition_t


// 异步写日志队列
// 模版代码，不能分文件写
template<typename T>
class LockQueue{
public:
    // 多个工作线程都会写日志队列
    void Push(const T& data){
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_cond.notify_one();       // 因为只有一个线程在从队列中取数据，所以没必要 notify_all
    }


    // 只有一个工作线程会从队列中取数据，然后执行IO操作
    T Pop(){
        std::unique_lock<std::mutex> lock(m_mutex);
        while(m_queue.empty()){
            // 日志队列为空，线程进入 wait 状态
            m_cond.wait(lock);  // 释放锁，进入 wait 状态，当被唤醒时会重新获取锁
        }

        T data = m_queue.front();
        m_queue.pop();

        return data;
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;     // condition_variable 一般需要和 unique_lock 联用
};


