// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_BASE_FILEUTIL_H
#define MUDUO_BASE_FILEUTIL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/StringPiece.h"
#include <sys/types.h> // for off_t

namespace muduo
{
    namespace FileUtil
    {

        // read small file < 64KB
        class ReadSmallFile : noncopyable
        {
        public:
            ReadSmallFile(StringArg filename);
            ~ReadSmallFile();

            // return errno
            template <typename String>
            int readToString(int maxSize,
                             String *content,
                             int64_t *fileSize,
                             int64_t *modifyTime,
                             int64_t *createTime);

            /// Read at maxium kBufferSize into buf_
            // return errno
            int readToBuffer(int *size);

            const char *buffer() const { return buf_; }

            static const int kBufferSize = 64 * 1024;

        private:
            int fd_;
            int err_;
            char buf_[kBufferSize];
        };

        // read the file content, returns errno if error happens.
        // 从文件读取数据保存到content中
        template <typename String>
        int readFile(StringArg filename,
                     int maxSize,
                     String *content,
                     int64_t *fileSize = NULL,
                     int64_t *modifyTime = NULL,
                     int64_t *createTime = NULL)
        {
            ReadSmallFile file(filename);
            return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
        }

        /*
          @brief: 写入数据
        */
        // not thread safe 非线程安全的，因为没加锁
        // 这个类隐藏着减少磁盘IO的秘密
        class AppendFile : noncopyable
        {
        public:
            explicit AppendFile(StringArg filename);
            ~AppendFile();

            // 添加字符串到文件
            void append(const char *logline, size_t len);
            void flush();
            off_t writtenBytes() const { return writtenBytes_; }

        private:
            size_t write(const char *logline, size_t len);

            FILE *fp_;               // 打开的文件指针
            char buffer_[64 * 1024]; // 文件缓冲区/用户态缓冲区 减少磁盘IO的次数
            off_t writtenBytes_;     // 已写入字节数
        };

    } // namespace FileUtil
} // namespace muduo

#endif // MUDUO_BASE_FILEUTIL_H
