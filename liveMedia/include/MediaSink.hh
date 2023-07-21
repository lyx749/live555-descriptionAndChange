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
// Media Sinks
// C++ header

#ifndef _MEDIA_SINK_HH
#define _MEDIA_SINK_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

/**
 * MediaSink类提供了一个基本的框架和接口，用于实现特定类型的媒体数据接收器。
 * 派生类可以继承这个类，并根据具体的需求实现continuePlaying()等方法，以实现特定类型的媒体数据接收和处理逻辑。
 */
class MediaSink : public Medium
{
public:
  static Boolean lookupByName(UsageEnvironment &env, char const *sinkName,
                              MediaSink *&resultSink);

  typedef void(afterPlayingFunc)(void *clientData);

  /// @brief 开始播放媒体数据
  /// @param source 需要播放的媒体源
  /// @param afterFunc 在播放结束后调用的afterFunc函数指针
  /// @param afterClientData afterFunc函数指针的参数
  /// @return success true, failed false
  Boolean startPlaying(MediaSource &source,
                       afterPlayingFunc *afterFunc,
                       void *afterClientData);

  /// @brief 停止播放
  virtual void stopPlaying();

  // Test for specific types of sink:
  virtual Boolean isRTPSink() const;

  FramedSource *source() const { return fSource; }

protected:
  MediaSink(UsageEnvironment &env); // abstract base class
  virtual ~MediaSink();

  virtual Boolean sourceIsCompatibleWithUs(MediaSource &source);
  // called by startPlaying()
  virtual Boolean continuePlaying() = 0;
  // called by startPlaying()

  /// @brief can be used in "getNextFrame()" calls，对成员函数onSourceClosure();的封装
  /// @param clientData 用于转换为该类指针的万能指针
  static void onSourceClosure(void *clientData);

  // should be called (on ourselves) by continuePlaying() when it
  // discovers that the source we're playing from has closed.
  void onSourceClosure();

  FramedSource *fSource;  //文件源

private:
  // redefined virtual functions:
  virtual Boolean isSink() const;

private:
  // The following fields are used when we're being played:
  afterPlayingFunc *fAfterFunc;
  void *fAfterClientData;
};

/**
 * 该类提供了一种用于管理输出数据包的缓冲区的机制。它允许向缓冲区中添加数据、提取数据、跳过字节等操作，
 * 并提供了一些方法用于管理溢出数据。通过使用该类，媒体接收器可以有效地管理输出数据包的缓冲和处理。
*/
class OutPacketBuffer
{
public:

  /// @brief 构造函数，计算缓冲区大小，初始化发送缓冲区
  /// @param preferredPacketSize 首选数据包大小
  /// @param maxPacketSize 最大数据包大小
  /// @param maxBufferSize 如果不设置，默认为60000
  OutPacketBuffer(unsigned preferredPacketSize, unsigned maxPacketSize,
                  unsigned maxBufferSize = 0);
  // if "maxBufferSize" is >0, use it - instead of "maxSize" to compute the buffer size
  ~OutPacketBuffer();

  static unsigned maxSize;


  /// @brief 增大发送缓冲区大小为newMaxSize
  /// @param newMaxSize 新的缓冲区大小
  static void increaseMaxSizeTo(unsigned newMaxSize)
  {
    if (newMaxSize > OutPacketBuffer::maxSize)
      OutPacketBuffer::maxSize = newMaxSize;
  }

  /// @brief 获取可以写入的缓冲区位置指针
  /// @return 返回当前可以缓冲区可写指针
  unsigned char *curPtr() const { return &fBuf[fPacketStart + fCurOffset]; }


  /// @brief 返回缓冲区中剩余可用字节数
  unsigned totalBytesAvailable() const
  {
    return fLimit - (fPacketStart + fCurOffset);
  }
  /// @brief 返回整个缓冲区大小
  unsigned totalBufferSize() const { return fLimit; }


  /// @brief 返回缓冲区中数据包的起始位置
  unsigned char *packet() const { return &fBuf[fPacketStart]; }


  /// @brief 返回当前缓冲区数据写入位置
  unsigned curPacketSize() const { return fCurOffset; }

  /// @brief 增加当前缓冲区指针的偏移量
  /// @param numBytes 需要增加的偏移量
  void increment(unsigned numBytes) { fCurOffset += numBytes; }

  /// @brief 将指定的字节数组添加到缓冲区的末尾
  /// @param from 需要添加的字符数组
  /// @param numBytes 需要添加的字节数
  void enqueue(unsigned char const *from, unsigned numBytes);

  /// @brief 将一个32位的无符号整数(先转换为网络字节序)添加到缓冲区的末尾。
  void enqueueWord(u_int32_t word);

  /// @brief 在缓冲区toPosition后的位置插入from字符数组的前numBytes个字节
  /// @param from 需要插入的字符数组
  /// @param numBytes 需要插入的字节数
  /// @param toPosition 需要插入的位置
  void insert(unsigned char const *from, unsigned numBytes, unsigned toPosition);

  /// @param word 将一个32位的无符号整数(先转换为网络字节序)添加到toPosition的后面。
  /// @param toPosition 需要添加的位置
  void insertWord(u_int32_t word, unsigned toPosition);

  /// @brief 从缓冲区fromPosition往后位置提取numBytes个字节的数据到to中
  void extract(unsigned char *to, unsigned numBytes, unsigned fromPosition);


  /// @brief 从缓冲区fromPosition处往后提取四个字节的数据，并且转换为主机字节序返回
  u_int32_t extractWord(unsigned fromPosition);

  /// @brief 跳过指定字节数，并且将fCurOffset移动相应字节数
  void skipBytes(unsigned numBytes);

  /// @brief 检查当前已经写好的缓冲区是否达到了首选数据包大小
  Boolean isPreferredSize() const { return fCurOffset >= fPreferred; }

  /// @brief 检查添加指定字节数后，是否会超过最大数据包大小
  Boolean wouldOverflow(unsigned numBytes) const
  {
    return (fCurOffset + numBytes) > fMax;
  }

   /// @brief 返回添加指定字节数后，最大数据包溢出的数据量。
  unsigned numOverflowBytes(unsigned numBytes) const
  {
    return (fCurOffset + numBytes) - fMax;
  }

  /// @brief 检查指定字节数是否超过了一个数据包的最大大小。
  Boolean isTooBigForAPacket(unsigned numBytes) const
  {
    return numBytes > fMax;
  }

  /// @brief 设置溢出数据消息
  /// @param overflowDataOffset 溢出数据在缓冲区中的偏移量
  /// @param overflowDataSize 溢出数据的大小
  /// @param presentationTime 溢出数据的呈现时间，使用struct timeval结构表示。
  /// @param durationInMicroseconds 溢出数据的持续时间，以微秒为单位
  void setOverflowData(unsigned overflowDataOffset,
                       unsigned overflowDataSize,
                       struct timeval const &presentationTime,
                       unsigned durationInMicroseconds);

                       
  /// @brief 返回溢出数据的大小
  unsigned overflowDataSize() const { return fOverflowDataSize; }

  /// @brief 溢出数据的呈现时间，使用struct timeval结构表示
  struct timeval overflowPresentationTime() const { return fOverflowPresentationTime; }

  /// @return 返回溢出数据的持续时间，以微秒为单位
  unsigned overflowDurationInMicroseconds() const { return fOverflowDurationInMicroseconds; }

  /// @brief 是否有溢出数据 
  Boolean haveOverflowData() const { return fOverflowDataSize > 0; }

  /// @brief 使用溢出数据
  void useOverflowData();

  void adjustPacketStart(unsigned numBytes);
  void resetPacketStart();
  void resetOffset() { fCurOffset = 0; }
  void resetOverflowData() { fOverflowDataOffset = fOverflowDataSize = 0; }

private:
  unsigned fPacketStart;  //表示数据包在缓冲区中的起始位置的偏移量
  unsigned fCurOffset;    //表示当前在缓冲区中的偏移量，即指示下一个数据将被写入或读取的位置
  unsigned fPreferred;    //首选数据包大小，表示缓冲区中的数据达到这个大小时，被认为是首选大小。
  unsigned fMax;          //最大数据包大小
  unsigned fLimit;        //缓冲区的限制大小，即缓冲区的总大小
  unsigned char *fBuf;    //缓冲区的指针，指向用于存储数据的内存块。

  unsigned fOverflowDataOffset; //溢出数据在缓冲区中的偏移量
  unsigned fOverflowDataSize;   //溢出数据的大小
  struct timeval fOverflowPresentationTime; //溢出数据的呈现时间，使用struct timeval结构表示。
  unsigned fOverflowDurationInMicroseconds; //溢出数据的持续时间，以微秒为单位
};

#endif
