// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/AsyncLogging.h"
#include "muduo/base/LogFile.h"
#include "muduo/base/Timestamp.h"

#include <stdio.h>

using namespace muduo;

AsyncLogging::AsyncLogging(const string &basename,
                           off_t rollSize,
                           int flushInterval)
    : flushInterval_(flushInterval),    // 刷新间隔，默认为3秒，注意一条消息的写入大概是1.2us左右
      running_(false),  // 用于线程终止
      basename_(basename),  
      rollSize_(rollSize),  // 一个文件中允许写入的最大字节数
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
    currentBuffer_->bzero();    // 清0
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len)
{
    // 可能多个线程往当前缓冲区中写数据，所以要加锁保护
    muduo::MutexLockGuard lock(mutex_); // 后端线程也抢这把锁，写日志
    if (currentBuffer_->avail() > len) // 如果当前缓冲剩余的空间足够大，则会把日志直接追加到缓冲中
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // 当前缓冲区已满，将当前缓冲区添加到 待写入文件的已填满的缓冲区
        buffers_.push_back(std::move(currentBuffer_));
        // 将预备缓冲区设置为当前缓冲区
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            // 这种情况，极少发生，前端写入速度太快，一下子把两块缓冲区(当前缓冲区和预备缓冲区)都写完了
            // 那么，只好分配一块新的缓冲区
            currentBuffer_.reset(new Buffer); // Rarely happens 很少发生
        }
        currentBuffer_->append(logline, len);
        cond_.notify(); // 通知后端开始写入日志，buffers_中有缓冲区加入了就通知
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_, rollSize_, false);
    //准备两块空闲缓冲区
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            muduo::MutexLockGuard lock(mutex_);
            // (注意，这里是一个非常规用法) //一般等待条件变量是使用while，
            //但是在muduo中只有一个日志线程，虚假唤醒也是没关系的，而且当超时后flushInterval_，也应该把日志写入文件中
            if (buffers_.empty()) // unusual usage!
            {
                cond_.waitForSeconds(flushInterval_);// 等待前端写满了一个或者多个buffer，或者一个超时时间到来
            }
            buffers_.push_back(std::move(currentBuffer_));// 将当前缓冲区移入buffers
            currentBuffer_ = std::move(newBuffer1);// 将空闲的newBffer1置为当前缓冲区
            buffersToWrite.swap(buffers_); // 内部指针交换，而非复制。这样后面的代码就可以在临界区之外安全地访问buffersToWrite，而不必一直对buffers加锁
            if (!nextBuffer_)
            {
                // 确保前端始终有一个预备buffer可供调配
                // 减少前端临界区分配内存的概率，缩短前端临界区的长度。
                // 如果没有预备buffer，就要重新new一个buffer, 所以要尽可能的减少影响前端线程并发的条件
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        // 消息堆积
        // 前端陷入死循环，拼命发送日志消息，超过后端的处理能力，这就是典型的生产速度超过消费速度的问题，
        // 会造成数据再内存中堆积，严重时引发性能问题：可用内存不足或程序崩溃(分配内存失败)。
        if (buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            // 只保留前两块，丢掉多余的日志，以腾出空间
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (const auto &buffer : buffersToWrite)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffer->data(), buffer->length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);   // 缩减为2块
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();    // 将缓冲区当前指针指向头部
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush(); // 写入文件
    }
    output.flush();
}
