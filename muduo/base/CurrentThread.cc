// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

namespace muduo
{
  namespace CurrentThread
  {
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char *t_threadName = "unknown";
    static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

    string stackTrace(bool demangle)
    {
      string stack;
      const int max_frames = 200;
      void *frame[max_frames];  // 指针数组，用于保存堆栈的地址
      int nptrs = ::backtrace(frame, max_frames); // 实际保存的个数

      // 将地址转换成函数名
      char **strings = ::backtrace_symbols(frame, nptrs); // backtrace_symbols 内部会调用malloc, 返回的指针需要由调用者释放
      if (strings)
      {
        size_t len = 256;
        char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr;
        for (int i = 1; i < nptrs; ++i) // skipping the 0-th, which is this function
        {
          if (demangle)
          {
            // https://panthema.net/2008/0901-stacktrace-demangled/
            // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
            char *left_par = nullptr;
            char *plus = nullptr;
            for (char *p = strings[i]; *p; ++p)
            {
              if (*p == '(')
                left_par = p;
              else if (*p == '+')
                plus = p;
            }

            if (left_par && plus)
            {
              *plus = '\0';
              int status = 0;
              char *ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
              *plus = '+';
              if (status == 0)
              {
                demangled = ret; // ret could be realloc()
                stack.append(strings[i], left_par + 1);
                stack.append(demangled);
                stack.append(plus);
                stack.push_back('\n');
                continue;
              }
            }
          }
          // Fallback to mangled names
          stack.append(strings[i]);
          stack.push_back('\n');
        }
        free(demangled);
        free(strings);  // 返回的是二级指针，调用者只需要释放外层指针所指向的资源，内层指针指向的是常量字符串(在常量区)不用释放
      }
      return stack;
    }

  } // namespace CurrentThread
} // namespace muduo
