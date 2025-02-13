#include "muduo/base/Logging.h"
#include "muduo/base/LogFile.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/base/TimeZone.h"

#include <stdio.h>
#include <unistd.h>

int g_total;
FILE *g_file;
std::unique_ptr<muduo::LogFile> g_logFile;

void dummyOutput(const char *msg, int len)
{
  g_total += len;
  if (g_file)
  {
    fwrite(msg, 1, len, g_file);
  }
  else if (g_logFile)
  {
    g_logFile->append(msg, len);
  }
}

void bench(const char *type)
{
  muduo::Logger::setOutput(dummyOutput);
  muduo::Timestamp start(muduo::Timestamp::now());
  g_total = 0;

  int n = 1000 * 1000;
  const bool kLongLog = false;
  muduo::string empty = " ";
  muduo::string longStr(3000, 'X');
  longStr += " ";
  for (int i = 0; i < n; ++i)
  {
    LOG_INFO << "Hello 0123456789"
             << " abcdefghijklmnopqrstuvwxyz"
             << (kLongLog ? longStr : empty)
             << i;
  }
  muduo::Timestamp end(muduo::Timestamp::now());
  double seconds = timeDifference(end, start);
  printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
         type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
  LOG_INFO << "logInThread";
  usleep(1000);
}

void test_muduo_logger()
{
  muduo::Logger::setLogLevel(muduo::Logger::ERROR);

  LOG_TRACE << "trace";
  LOG_DEBUG << "debug";
  LOG_INFO << "Hello";
  LOG_WARN << "World";
  LOG_ERROR << "Error";
  LOG_SYSERR << "SYSERR";
  // LOG_SYSFATAL << "SYSFATAL";
  // LOG_FATAL << "FATAL";
  LOG_INFO << "sizeof(muduo::Logger):" << sizeof(muduo::Logger);
  LOG_INFO << "sizeof(muduo::LogStream):" << sizeof(muduo::LogStream);
  LOG_INFO << "sizeof(muduo::Fmt):" << sizeof(muduo::Fmt);
  LOG_INFO << "sizeof(muduo::LogStream::Buffer): " << sizeof(muduo::LogStream::Buffer);
}

void test_muduo_logger_inThread()
{
  muduo::ThreadPool pool("pool");
  pool.start(5);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
}

void test_bench_dev_null()
{
  char buffer[64 * 1024];

  g_file = fopen("/dev/null", "w");
  setbuffer(g_file, buffer, sizeof buffer);
  bench("/dev/null");
  fclose(g_file);
}

void test_bench_tmp_log()
{
  char buffer[64 * 1024];

  g_file = fopen("/tmp/log", "w");
  setbuffer(g_file, buffer, sizeof buffer);
  bench("/tmp/log");
  fclose(g_file);
}

// 非线程安全
void test_bench_log_st()
{
  g_file = NULL;
  g_logFile.reset(new muduo::LogFile("test_log_st", 500 * 1000 * 1000, false));
  bench("test_log_st");
}

// 线程安全
void test_bench_log_mt()
{
  g_logFile.reset(new muduo::LogFile("test_log_mt", 500 * 1000 * 1000, true));
  bench("test_log_mt");
  g_logFile.reset();
}

void test_logger_TimeZone()
{
  {
    g_file = stdout;
    sleep(1);
    muduo::TimeZone beijing(8 * 3600, "CST");
    muduo::Logger::setTimeZone(beijing);
    LOG_TRACE << "trace CST";
    LOG_DEBUG << "debug CST";
    LOG_INFO << "Hello CST";
    LOG_WARN << "World CST";
    LOG_ERROR << "Error CST";

    sleep(1);
    muduo::TimeZone newyork("/usr/share/zoneinfo/America/New_York");
    muduo::Logger::setTimeZone(newyork);
    LOG_TRACE << "trace NYT";
    LOG_DEBUG << "debug NYT";
    LOG_INFO << "Hello NYT";
    LOG_WARN << "World NYT";
    LOG_ERROR << "Error NYT";
    g_file = NULL;
  }
}

int main()
{
  getppid(); // for ltrace and strace

  test_muduo_logger();
  // test_muduo_logger_inThread();

  // bench("nop");
  // test_bench_dev_null();
  // test_bench_tmp_log();
  // test_bench_log_st();
  // test_bench_log_mt();

  // test_logger_TimeZone();
  // bench("timezone nop");
}
