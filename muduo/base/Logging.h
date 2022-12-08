// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include "muduo/base/LogStream.h"
#include "muduo/base/Timestamp.h"

namespace muduo
{

  class TimeZone;

  class Logger
  {
  public:
    enum LogLevel
    {
      TRACE,
      DEBUG,
      INFO,
      WARN,
      ERROR,
      FATAL,
      NUM_LOG_LEVELS,
    };

    /*
      @brief: 处理传入的路径
    */ 
    // compile time calculation of basename of source file
    class SourceFile
    {
    public:
      template <int N>  // 非类型模板参数 构造函数可隐式转换
      SourceFile(const char (&arr)[N])  // 匹配字符数组写法 因为是引用 效率更高一点
          : data_(arr),
            size_(N - 1)
      { // 找到路径中的文件名， 返回第二个参数最后一次出现的位置的指针
        const char *slash = strrchr(data_, '/'); // builtin function
        if (slash)
        {
          data_ = slash + 1;
          size_ -= static_cast<int>(data_ - arr);
        }
      }

      explicit SourceFile(const char *filename)
          : data_(filename)
      {
        const char *slash = strrchr(filename, '/');
        if (slash)
        {
          data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
      }

      const char *data_;
      int size_;
    };

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char *func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream &stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    typedef void (*OutputFunc)(const char *msg, int len);
    typedef void (*FlushFunc)();
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);
    static void setTimeZone(const TimeZone &tz);

  private:
    class Impl
    {
    public:
      typedef Logger::LogLevel LogLevel;
      Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
      void formatTime();
      void finish();

      Timestamp time_;  // 提供当前时间 可得到1970年到现在的毫秒数与生成标准时间字符串
      LogStream stream_;
      LogLevel level_;  // 当前日志级别
      int line_;  // 日志行数 由__line__得到
      SourceFile basename_; // 日志所属文件名 由__file__与SourceFile类得到
    };

    Impl impl_;
  };

  extern Logger::LogLevel g_logLevel;

  inline Logger::LogLevel Logger::logLevel()
  {
    return g_logLevel;
  }

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

  const char *strerror_tl(int savedErrno);

  // Taken from glog/logging.h
  //
  // Check that the input is non NULL.  This very useful in constructor
  // initializer lists.

#define CHECK_NOTNULL(val) \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

  // A small helper for CHECK_NOTNULL().
  template <typename T>
  T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
  {
    if (ptr == NULL)
    {
      Logger(file, line, Logger::FATAL).stream() << names;
    }
    return ptr;
  }

} // namespace muduo

#endif // MUDUO_BASE_LOGGING_H
