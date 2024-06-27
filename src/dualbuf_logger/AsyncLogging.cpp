
#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"



AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1)
{
    assert(logFileName_.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}



/*
函数功能：前端生成一条日志信息
    buffers_            前台缓冲队列。用来存放积攒的日志消息
    currentBuffer_      当前缓冲区。追加的日志消息存放于此
    nextBuffer_         预备缓冲区。主要是减少内存开销

函数逻辑：
    * 若当前缓冲区未满，追加日志消息到当前缓冲区
    * 若当前缓冲区写满，首先，把它移入前台缓冲队列 buffers_。其次，尝试把预备缓冲区 nextBuffer_ 移用为当前缓冲区
      如果移用失败（前台瞬间将两个缓冲区写满），则创建新的缓冲区作为当前缓冲。最后，追加日志消息并唤醒后端日志线程开始写入数据。
*/
void AsyncLogging::append(const char *logline, int len)
{
    MutexLockGuard lock(mutex_);                // 多线程加锁，保证线程安全

    // 判断当前缓冲区是否已经写满
    if (currentBuffer_->avail() > len)
        // 1. 当前缓冲区没有写满
        currentBuffer_->append(logline, len);
    else
    {
        // 2. 当前缓冲区写满。
        // 其一、将写满的当前缓冲区的日志消息写入到缓冲队列 buffers_
        // 其二、追加日志消息到当前缓冲，唤醒后台日志落盘线程

        // 其一
        buffers_.push_back(currentBuffer_);    
        currentBuffer_.reset();

        if (nextBuffer_)                        // 预备缓冲区未满时，复用该缓冲区
            currentBuffer_ = std::move(nextBuffer_);
        else                                    // 预备缓冲区也写满（极少发生），重新分配缓冲
            currentBuffer_.reset(new Buffer);

        // 其二
        currentBuffer_->append(logline, len);
        cond_.notify();
    }
}



/*
函数功能：负责日志的落地相关
    buffersToWrite        后台缓冲队列   
    newBuffer1            备用缓冲区1。用来替换前台的当前缓冲区
    newBuffer2            备用缓冲区2。用来替换前台的预备缓冲区（这种反复替换利用，减少了内存开销）

函数逻辑：
    * 唤醒日志落盘线程（超时或者写满buffer），交换前台缓冲队列和后台缓冲队列。加锁
    * 日志落盘，将后台缓冲队列的所有buffer写入文件。不加锁，不加锁的好处是日志落盘不影响浅谈缓冲队列的插入，不会出现阻塞问题，极大的提高了系统性能。
*/
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    // logFile 类负责将数据写入磁盘
    LogFile output(basename_);      

    BufferPtr newBuffer1(new Buffer);   // 用来替换前台的当前缓冲 currentbuffer_
    BufferPtr newBuffer2(new Buffer);   // 用来替换前台的预备缓冲 nextbuffer_
    newBuffer1->bzero();
    newBuffer2->bzero();

    BufferVector buffersToWrite;        // 后台缓冲队列
    buffersToWrite.reserve(16);         // 调整预留空间，主要影响的是 capacity

    // 异步日志开启，则循环执行
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        // <---------- 交换前台缓冲队列和后台缓冲队列 ---------->
        {   // 锁的作用域。如果放在外面，会增加锁的粒度(锁能控制的代码块大小)，即日志落盘的时候都会阻塞 append
            // 1. 多线程加锁，线程安全，注意锁的作用域
            MutexLockGuard lock(mutex_);

            // 2. 判断前台缓冲队列 buffers 是否有数据可读
            if (buffers_.empty()) {
                // buffers_ 没有数据可读，休眠（唤醒条件：1.超时 or 2.前台写满buffer）
                cond_.waitForSeconds(flushInterval_);   // 内部封装了 pthread_cond_timedwait，暂时释放锁，等待条件变量置位
            }

            // 3. 将当前缓冲区 currentbuffer 移入前台缓冲队列 buffers_
            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();

            // 4. 将空闲的 newbuffer1 移位当前缓冲区，复用已经分配的空间，减少资源消耗
            currentBuffer_ = std::move(newBuffer1);

            // 5. 核心：把前台缓冲队列的所有buffer交换（交换 buffers_ 和 buffersToWrite），这样后续的日志落盘过程中不影响前台缓冲队列的插入
            buffersToWrite.swap(buffers_);

            // 若预备缓冲为空，则将空闲的 newbuffer2 移为预备缓冲，复用已分配的空间
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }   // 这里注意加锁的粒度，日志落盘的时候不需要加锁，主要是双队列的功劳

        // <---------- 日志落盘，将buffersToWrite中的所有buffer写入文件 ---------->
        assert(!buffersToWrite.empty());

        if (buffersToWrite.size() > 25)
        {
            // 插入提示信息
            // char buf[256];
            // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
            // buffers\n",
            //          Timestamp::now().toFormattedString().c_str(),
            //          buffersToWrite.size()-2);
            // fputs(buf, stderr);
            // output.append(buf, static_cast<int>(strlen(buf)));
            
            // 只保留2个buffer（默认4M）
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        // 7. 循环写入 buffersToWrite 的所有 buffer
        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // 内部封装 fwrite，将 buffers 中的一行日志数据，写入用户缓冲区，等待写入文件
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }
        
        // 9. 重新填充 newBuffer1 和 newBuffer2
        // 改变后台缓冲队列的大小，始终只保存两个 buffer，多余的 buffer 被释放
        // 为什么不直接保存到当前和预备缓冲？这是因为加锁的粒度，二者需要加锁操作
        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);   // 分别用于填充备用缓冲 newBuffer1 和 newBuffer2
        }

        // 用 buffersToWrite 内的 buffer 重新填充 newBuffer1
        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back(); // 复用 buffer
            buffersToWrite.pop_back();
            newBuffer1->reset();                // 重置指针，置空
        }

        // 用 buffersToWrite 内的 buffer 重新填充 newBuffer2
        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back(); // 复用 buffer
            buffersToWrite.pop_back();
            newBuffer2->reset();                // 重置指针，置空
        }

        // 清空 buffersToWrite
        buffersToWrite.clear();
        output.flush();
    }

    // 存在问题？
    output.flush();
}

