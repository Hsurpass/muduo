// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/LogFile.h"

#include "muduo/base/FileUtil.h"
#include "muduo/base/ProcessInfo.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

LogFile::LogFile(const string &basename,
                 off_t rollSize,
                 bool threadSafe, // 线程安全控制项, 默认为true. 当只有一个后端AsnycLogging和后端线程时, 该项可置为false
                 int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),           // 一个最大刷新字节数
      flushInterval_(flushInterval), // 刷新间隔
      checkEveryN_(checkEveryN),     // 允许停留在buffer的最大日志条数，
      count_(0),                     // 写数据次数计数, 超过限值checkEveryN_时清除, 然后重新计数
      mutex_(threadSafe ? new MutexLock : NULL),
      startOfPeriod_(0), // 记录前一天的时间，单位:秒
      lastRoll_(0),      // 上一次滚动日志文件的时间，单位：秒
      lastFlush_(0)      // 上一次刷新的时间，单位:秒
{
    assert(basename.find('/') == string::npos); // 判断文件名是否合法，basename是不包含 '/' 的
    rollFile(); // 构造时先产生一个文件
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len)
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else
    {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else
    {
        file_->flush();
    }
}

void LogFile::append_unlocked(const char *logline, int len)
{
    file_->append(logline, len);

    if (file_->writtenBytes() > rollSize_) // 已写的字节数是否大于规定的文件大小
    {
        rollFile();
    }
    else
    {
        ++count_; // 记录写入的日志条数
        if (count_ >= checkEveryN_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod_ != startOfPeriod_) // 天数不相同, 重新生成文件
            {
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_)
            { // 如果现在和上次刷新缓冲区的时间差超过flushInterval_，进行刷新
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

bool LogFile::rollFile()
{
    time_t now = 0;
    // 可以由now得到标准时间 即Unix时间戳.是自1970/1/1 00:00:00GMT 以来的秒数
    string filename = getLogFileName(basename_, &now);  // 得到完整的日志文件名
    // 注意，这里先除kRollPerSeconds_后乘kRollPerSeconds_表示对齐至kRollPerSeconds_整数倍，
    // 也就是时间调整到当天零点
    // now / kRollPerSeconds_ * kRollPerSeconds_不一定等于 now,因为now / kRollPerSeconds_ 得到的是整数， 所以这个表达式的含义就是求得kRollPerSeconds_的整数倍
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) // 若当前时间大于上次滚动的时间lastRoll_
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start; // 记录上一次rollfile的日期(天)
        // 换一个文件写日志，即为了保证两天的日志不写在同一个文件中, 而上一天的日志可能并未写到rollSize_大小
        file_.reset(new FileUtil::AppendFile(filename)); // 打开一个新的日志文件
        return true;
    }
    return false;
}

string LogFile::getLogFileName(const string &basename, time_t *now)
{
    string filename;
    filename.reserve(basename.size() + 64); // basename后面在格式化为字符串后最大为64
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm); // FIXME: localtime_r ? // gmtime_r获取的是gmt时区时间，localtime_r获取的是本地时间。
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;    // +时间戳

    filename += ProcessInfo::hostname();    // +主机名

    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    filename += pidbuf; // +pid

    filename += ".log"; // +.log

    return filename;
}
