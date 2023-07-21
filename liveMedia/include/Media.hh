/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// Medium
// C++ header

#ifndef _MEDIA_HH
#define _MEDIA_HH

#ifndef _LIVEMEDIA_VERSION_HH
#include "liveMedia_version.hh"
#endif

#ifndef _HASH_TABLE_HH
#include "HashTable.hh"
#endif

#ifndef _USAGE_ENVIRONMENT_HH
#include "UsageEnvironment.hh"
#endif

// Lots of files end up needing the following, so just #include them here:
#ifndef _NET_COMMON_H
#include "NetCommon.h"
#endif
#include <stdio.h>

// The following makes the Borland compiler happy:
#ifdef __BORLANDC__
#define _strnicmp strnicmp
#define fabsf(x) fabs(x)
#endif

#define mediumNameMaxLen 30 // 名字最大长度

/// @brief 该类为所有的liveMedia文件夹中的类的基类
class Medium
{
public:
  //!!<静态方法> 通过名字从MediaLookupTable中查找对应的Medium通过resultMedium返回，然后查找成功返回true，失败返回false
  static Boolean lookupByName(UsageEnvironment &env,
                              char const *mediumName,
                              Medium *&resultMedium);

  //!!<静态方法> 通过名字从MediaLookupTable中查找对应的Medium，并且从MediaLookupTable中移除
  static void close(UsageEnvironment &env, char const *mediumName);

  //!!<静态方法> 从MediaLookupTable移除medium，回调static void close(UsageEnvironment& env, char const* mediumName)函数
  static void close(Medium *medium); // alternative close() method using ptrs
                                     // (has no effect if medium == NULL)

  // 返回fEnviorn(UsageEnvironment)
  UsageEnvironment &envir() const { return fEnviron; }

  // 返回fMediumName(char const*)
  char const *name() const { return fMediumName; }

  // Test for specific types of media:
  virtual Boolean isSource() const;
  virtual Boolean isSink() const;
  virtual Boolean isRTCPInstance() const;
  virtual Boolean isRTSPClient() const;
  virtual Boolean isRTSPServer() const;
  virtual Boolean isMediaSession() const;
  virtual Boolean isServerMediaSession() const;

protected:
  friend class MediaLookupTable;
  Medium(UsageEnvironment &env); // abstract base class
  virtual ~Medium();             // instances are deleted using close() only

  TaskToken &nextTask()
  {
    return fNextTask;
  }

private:
  UsageEnvironment &fEnviron;         // 用户基础环境变量，保存着任务调度器，和错误打印
  char fMediumName[mediumNameMaxLen]; // Medium名
  TaskToken fNextTask;                // 下一个任务，TaskToken为void*类型
};

// A data structure for looking up a Medium by its string name.
// (It is used only to implement "Medium", but we make it visible here, in case developers want to use it to iterate over
//  the whole set of "Medium" objects that we've created.)
class MediaLookupTable
{
public:
  /// @brief 传入环境变量env，构造MediaLookupTable(静态方法)
  /// @param env 用户基础环境变量
  /// @return 构造的MediaLookupTable指针
  static MediaLookupTable *ourMedia(UsageEnvironment &env);

  /// @brief 得到Media的hashTable
  /// @return MediaTable
  HashTable const &getTable() { return *fTable; }

protected:
  /// @brief 传入env构造MediaLookupTable(安全的，构造方法被声明为protected，只能通过上面的静态方法构建)
  /// @param env
  MediaLookupTable(UsageEnvironment &env);
  virtual ~MediaLookupTable();

private:
  friend class Medium;

  Medium *lookup(char const *name) const;
  // Returns NULL if none already exists

  void addNew(Medium *medium, char *mediumName);
  void remove(char const *name);

  /// @brief 迭代生成media的名字
  /// @param mediumName 用来接收生成的名字
  /// @param maxLen media名字的最大长度
  void generateNewName(char *mediumName, unsigned maxLen);

private:
  UsageEnvironment &fEnv;  // 用户基础环境配置
  HashTable *fTable;       // 以mediumName为key，Medium的地址为value的哈希表
  unsigned fNameGenerator; // 命名计数器
};

// The structure pointed to by the "liveMediaPriv" UsageEnvironment field:
class _Tables
{
public:
  // returns a pointer to a "_Tables" structure (creating it if necessary)
  static _Tables *getOurTables(UsageEnvironment &env, Boolean createIfNotPresent = True);

   // used to delete ourselves when we're no longer used_Tables
  void reclaimIfPossible();
 

  MediaLookupTable *mediaTable;
  void *socketTable;

protected:
  _Tables(UsageEnvironment &env);
  virtual ~_Tables();

private:
  UsageEnvironment &fEnv;
};

#endif
