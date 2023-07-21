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
// Framed Sources
// C++ header

#ifndef _FRAMED_SOURCE_HH
#define _FRAMED_SOURCE_HH

#ifndef _NET_COMMON_H
#include "NetCommon.h"
#endif
#ifndef _MEDIA_SOURCE_HH
#include "MediaSource.hh"
#endif

/**
 * 这个类是一个抽象基类，用于表示帧式数据源。它提供了一组方法和变量，用于获取帧数据、处理事件和存储相关的参数。
 * 具体的数据源子类需要继承该基类，并实现纯虚函数 doGetNextFrame() 来实现具体的数据获取逻辑
 */
class FramedSource : public MediaSource
{
public:
  /// @brief 从基类MediaTable中根据名字寻找
  /// @param env 用户基础环境变量
  /// @param sourceName 需要查找的名字
  /// @param resultSource 返回的结果
  /// @return success true， failed false
  static Boolean lookupByName(UsageEnvironment &env, char const *sourceName,
                              FramedSource *&resultSource);

  typedef void(afterGettingFunc)(void *clientData, unsigned frameSize,
                                 unsigned numTruncatedBytes,
                                 struct timeval presentationTime,
                                 unsigned durationInMicroseconds);
  typedef void(onCloseFunc)(void *clientData);

  /// @brief 用于获取帧数据的关键函数。它接受目标缓冲区和相关的回调函数作为参数，
  ///        并负责从数据源中获取帧数据并进行相应的处理。在成功获取帧数据后，可以通过 afterGettingFunc 回调函数进行进一步的处理，
  ///        而在数据源关闭时，可以通过 onCloseFunc 回调函数进行适当的处理。
  /// @param to 指向目标缓冲区的指针，用于存储获取的帧数据
  /// @param maxSize 目标缓冲区的最大大小，用于指定缓冲区的容量
  /// @param afterGettingFunc 一个函数指针，指向一个函数，该函数在成功获取帧数据后被调用
  /// @param afterGettingClientData 一个指针，用于传递给 afterGettingFunc 函数的客户端数据
  /// @param onCloseFunc 一个函数指针，指向一个函数，该函数在数据源关闭时被调用
  /// @param onCloseClientData 一个指针，用于传递给 onCloseFunc 函数的客户端数据。
  void getNextFrame(unsigned char *to, unsigned maxSize,
                    afterGettingFunc *afterGettingFunc,
                    void *afterGettingClientData,
                    onCloseFunc *onCloseFunc,
                    void *onCloseClientData);

  /// @brief 一个静态封装函数，调用成员函数handleClosure(),
  ///        This should be called (on ourself) if the source is discovered
  ///        to be closed (i.e., no longer readable)
  /// @param clientData 当前类指针，用于调用成员函数
  static void handleClosure(void *clientData);

  /// @brief  被静态函数handleClosure(void *clientData)调用
  void handleClosure();

  virtual unsigned maxFrameSize() const;
  // size of the largest possible frame that we may serve, or 0
  // if no such maximum is known (default)

  // 将相关标志和回调函数重置为初始状态，然后调用 doStopGettingFrames()，
  // 该函数执行停止获取帧数据的默认操作，包括取消任何挂起的传递任务。
  // 子类可以根据需要重新定义 doStopGettingFrames() 函数，以执行特定的停止获取帧数据的操作。
  void stopGettingFrames();

  // called by getNextFrame()
  virtual void doGetNextFrame() = 0;

  Boolean isCurrentlyAwaitingData() const { return fIsCurrentlyAwaitingData; }

  static void afterGetting(FramedSource *source);
  // doGetNextFrame() should arrange for this to be called after the
  // frame has been read (*iff* it is read successfully)

protected:
  FramedSource(UsageEnvironment &env); // abstract base class
  virtual ~FramedSource();

  virtual void doStopGettingFrames();

protected:
  // The following variables are typically accessed/set by doGetNextFrame()
  unsigned char *fTo;               // 指向目标缓冲区的指针
  unsigned fMaxSize;                // 目标缓冲区的最大大小
  unsigned fFrameSize;              // 输出参数，用于存储实际帧的大小
  unsigned fNumTruncatedBytes;      // 输出参数，用于存储被截断的字节数
  struct timeval fPresentationTime; // 输出参数，用于存储帧的呈现时间
  unsigned fDurationInMicroseconds; // 输出参数，用于存储帧的持续时间

private:
  // redefined virtual functions:
  virtual Boolean isFramedSource() const;

private:
  afterGettingFunc *fAfterGettingFunc; // 用于存储获取帧后的回调函数
  void *fAfterGettingClientData;       // 用于存储获取帧相关的客户端数据
  onCloseFunc *fOnCloseFunc;           // 用于存储关闭事件的回调函数
  void *fOnCloseClientData;            // 用于存储关闭事件相关的客户端数据

  Boolean fIsCurrentlyAwaitingData; // 一个布尔值，用于指示当前是否正在等待数据
};

#endif
