// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include "muduo/base/BlockingQueue.h"
#include "muduo/base/BoundedBlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/LogStream.h"

#include <atomic>
#include <vector>

namespace muduo
{

    class AsyncLogging : noncopyable
    {
    public:
        AsyncLogging(const string &basename,
                     off_t rollSize,
                     int flushInterval = 3);

        ~AsyncLogging()
        {
            if (running_)
            {
                stop();
            }
        }

        // 供前端生产者线程调用(日志数据写到缓冲区)
        void append(const char *logline, int len);

        void start()
        {
            running_ = true;
            thread_.start();    // 日志线程启动
            latch_.wait();  // 主线程等待子线程启动后再往下执行
        }

        void stop() NO_THREAD_SAFETY_ANALYSIS
        {
            running_ = false;
            cond_.notify();
            thread_.join();
        }

    private:
        void threadFunc();  // 供后端消费者线程调用，将数据写到日志文件中

        typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer; // 固定大小的缓冲区
        typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
        typedef BufferVector::value_type BufferPtr;

        const int flushInterval_;//超过时间，在flushInterval_秒内，缓冲区没写满，仍将缓冲区中的数据写到文件中
        std::atomic<bool> running_;
        const string basename_;
        const off_t rollSize_;  // 日志文件的滚动大小

        muduo::Thread thread_;
        muduo::CountDownLatch latch_;//用于等待线程启动，让子线程先启动

        muduo::MutexLock mutex_;
        muduo::Condition cond_ GUARDED_BY(mutex_);
        
        BufferPtr currentBuffer_ GUARDED_BY(mutex_);//当前缓冲区
        BufferPtr nextBuffer_ GUARDED_BY(mutex_);   //预备缓冲区
        BufferVector buffers_ GUARDED_BY(mutex_);   //待写入文件的已填满的缓冲区，也可以是没填满的(到时的)
    };

} // namespace muduo

#endif // MUDUO_BASE_ASYNCLOGGING_H
