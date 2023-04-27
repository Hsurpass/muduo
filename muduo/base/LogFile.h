// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include "muduo/base/Mutex.h"
#include "muduo/base/Types.h"

#include <memory>

namespace muduo
{

    namespace FileUtil
    {
        class AppendFile;   // 前向声明
    }

    /*
      @brief:生成文件名
    */
    class LogFile : noncopyable
    {
    public:
        LogFile(const string &basename,
                off_t rollSize,         // 一次最大刷新字节数
                bool threadSafe = true, // 通过对写入操作加锁，来决定是否线程安全
                int flushInterval = 3,  // 隔多少毫秒刷新一次
                int checkEveryN = 1024);// 允许写入的最大条数
        ~LogFile();

        void append(const char *logline, int len);
        void flush();   // 清空缓冲区
        bool rollFile();    // 滚动日志

    private:
        void append_unlocked(const char *logline, int len);

        static string getLogFileName(const string &basename, time_t *now);  // 获取日志文件的名称

        const string basename_;   // 日志文件basename
        const off_t rollSize_;    // 日志文件达到rollSize_ 换一个新文件
        const int flushInterval_; // 日志写入时间间隔
        const int checkEveryN_;

        int count_;

        std::unique_ptr<MutexLock> mutex_;
        time_t startOfPeriod_; // 开始记录日志时间(调整至零点)
        time_t lastRoll_;      // 上一次滚动日志文件时间
        time_t lastFlush_;     // 上一次日志写入时间
        std::unique_ptr<FileUtil::AppendFile> file_;

        const static int kRollPerSeconds_ = 60 * 60 * 24; // 一天的秒数
    };

} // namespace muduo
#endif // MUDUO_BASE_LOGFILE_H
