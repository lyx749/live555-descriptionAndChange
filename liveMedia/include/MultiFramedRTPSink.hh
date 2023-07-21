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
// RTP sink for a common kind of payload format: Those which pack multiple,
// complete codec frames (as many as possible) into each RTP packet.
// C++ header

#ifndef _MULTI_FRAMED_RTP_SINK_HH
#define _MULTI_FRAMED_RTP_SINK_HH

#ifndef _RTP_SINK_HH
#include "RTPSink.hh"
#endif

/// @brief 这是一个多帧的RTP发送器类，它是RTPSink的子类。该类用于将多个帧（数据包）组装成RTP包并发送到指定的目的地。
class MultiFramedRTPSink : public RTPSink
{
public:
  /// @brief 删除原有的OutPacketBuffer，然后重新new一个
  /// @param preferredPacketSize OutPacketBuffer需要的参数，表示首选数据包大小
  /// @param maxPacketSize OutPacketBuffer需要的参数，表示 最大数据包大小
  void setPacketSizes(unsigned preferredPacketSize, unsigned maxPacketSize);

  typedef void(onSendErrorFunc)(void *clientData);

  /// @brief 设置发送错误处理函数
  void setOnSendErrorFunc(onSendErrorFunc *onSendErrorFunc, void *onSendErrorFuncData)
  {
    // Can be used to set a callback function to be called if there's an error sending RTP packets on our socket.
    fOnSendErrorFunc = onSendErrorFunc;
    fOnSendErrorData = onSendErrorFuncData;
  }

protected:
  /// @brief 所有参数都是用来构造RTPSink类的，调用RTPsink构造函数之后就设置自身变量的初始值，然后调用setPacketSizes初始化发送缓冲区类
  MultiFramedRTPSink(UsageEnvironment &env,
                     Groupsock *rtpgs, unsigned char rtpPayloadType,
                     unsigned rtpTimestampFrequency,
                     char const *rtpPayloadFormatName,
                     unsigned numChannels = 1);
  // we're a virtual base class

  virtual ~MultiFramedRTPSink();

  /// @brief 具体实现在派生类中，在这只做了默认实现，就是判断这是不是第一帧，如果是就把当前帧呈现的时间设置为RTP包的时间戳
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char *frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);
  // perform any processing specific to the particular payload format

  /// @brief 控制是否允许在RTP包中对帧进行分片
  virtual Boolean allowFragmentationAfterStart() const;
  // whether a frame can be fragmented if other frame(s) appear earlier
  // in the packet (by default: False)

  // whether other frames can be packed into a packet following the
  // final fragment of a previous, fragmented frame (by default: False)
  /// @brief 是否允许在最后一个分片后添加其他帧
  virtual Boolean allowOtherFramesAfterLastFragment() const;

  // whether this frame can appear in position >1 in a pkt (default: True)
  virtual Boolean frameCanAppearAfterPacketStart(unsigned char const *frameStart,
                                                 unsigned numBytesInFrame) const;

  // returns the size of any special header used (following the RTP header) (default: 0)
  /// @brief 回特殊头部大小，用于创建RTP包。默认为0
  virtual unsigned specialHeaderSize() const;

  // returns the size of any frame-specific header used (before each frame
  // within the packet) (default: 0)
  virtual unsigned frameSpecificHeaderSize() const;

  // returns the number of overflow bytes that would be produced by adding a new
  // frame of size "newFrameSize" to the current RTP packet.
  // (By default, this just calls "numOverflowBytes()", but subclasses can redefine
  // this to (e.g.) impose a granularity upon RTP payload fragments.)
  /// @brief 计算添加新帧后的溢出字节数。溢出字节表示新帧加入RTP包后，RTP包是否会超过最大包大小
  virtual unsigned computeOverflowForNewFrame(unsigned newFrameSize) const;

  // Functions that might be called by doSpecialFrameHandling(), or other subclass virtual functions:

  /// @brief 返回当前RTP包是否是第一个包，如果为True，则表示当前RTP包是新的一组帧的第一个包。
  Boolean isFirstPacket() const { return fIsFirstPacket; }

  /// @brief 返回当前帧是不是RTP包的第一帧
  Boolean isFirstFrameInPacket() const { return fNumFramesUsedSoFar == 0; }

  /// @brief 返回当前帧的分片偏移量。当一个帧需要分片时，此变量记录当前帧数据在RTP包中的偏移量。
  unsigned curFragmentationOffset() const { return fCurFragmentationOffset; }

  /// @brief 将RTP数据报头部的M标记位设置为1
  void setMarkerBit();

  /// @brief 先将framePresentationTime转换为32位的RTP数据头部的时间戳格式，然后将其插入到RTP数据报头部中
  void setTimestamp(struct timeval framePresentationTime);

  /// @brief 设定特定头部字<将word插入到RTP数据包头部的特殊头部的wordPosition位置>
  /// @param word 需要插入的word
  /// @param wordPosition 相对于特殊头部起始位置的位置
  void setSpecialHeaderWord(unsigned word, /* 32 bits, in host order */
                            unsigned wordPosition = 0);

  /// @brief 设定特定头部字节序列<将bytes的前numBytes个字节插入到RTP数据包头部的特殊头部的bytePosition位置>
  void setSpecialHeaderBytes(unsigned char const *bytes, unsigned numBytes,
                             unsigned bytePosition = 0);

  /// @brief 设置特定帧头部的字 
  void setFrameSpecificHeaderWord(unsigned word, /* 32 bits, in host order */
                                  unsigned wordPosition = 0);

  /// @brief 设定特定帧头部的字节序
  void setFrameSpecificHeaderBytes(unsigned char const *bytes, unsigned numBytes,
                                   unsigned bytePosition = 0);

  /// @brief 设置帧的填充字节
  void setFramePadding(unsigned numPaddingBytes);

  /// @brief 返回当前已经使用的帧数。用于跟踪当前RTP包中已经包含的帧数量。
  unsigned numFramesUsedSoFar() const { return fNumFramesUsedSoFar; }

  /// @brief 返回当前RTP包的最大大小，以字节为单位 
  unsigned ourMaxPacketSize() const { return fOurMaxPacketSize; }

public: // redefined virtual functions:
  /// @brief 停止发送RTP数据包
  virtual void stopPlaying();

protected: // redefined virtual functions:
  /// @brief 继续发送数据包
  virtual Boolean continuePlaying();

private:

  /// @brief 构建并且发送RTP数据包(这个函数主要是构建包头，包体在内部调用packFrame()进行构造)
  void buildAndSendPacket(Boolean isFirstPacket);

  /// @brief 开始将尽可能多的（完整的）帧打包进数据包中
  void packFrame();

  /// @brief 发送RTP数据包
  void sendPacketIfNecessary();

  static void sendNext(void *firstArg);

  friend void sendNext(void *);

  static void afterGettingFrame(void *clientData,
                                unsigned numBytesRead, unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned numBytesRead, unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);
  Boolean isTooBigForAPacket(unsigned numBytes) const;

  static void ourHandleClosure(void *clientData);

private:
  OutPacketBuffer *fOutBuf; // 用于管理RTP包的发送缓冲区。

  Boolean fNoFramesLeft;                    // 表示是否没有剩余的帧需要发送。如果为True，则表示已经发送了所有的帧，不再有待发送的帧数据。
  unsigned fNumFramesUsedSoFar;             // 记录当前已经使用的帧数。用于跟踪当前RTP包中已经包含的帧数量。
  unsigned fCurFragmentationOffset;         // 当前帧的分片偏移量。当一个帧需要分片时，此变量记录当前帧数据在RTP包中的偏移量。
  Boolean fPreviousFrameEndedFragmentation; // 表示上一个帧是否已经结束分片。如果为True，则表示上一个帧已经分片结束，当前帧需要在RTP包中新开一个分片

  Boolean fIsFirstPacket;                   // 表示当前RTP包是否是第一个包。如果为True，则表示当前RTP包是新的一组帧的第一个包。
  struct timeval fNextSendTime;             // 记录下一个RTP包的发送时间。用于控制RTP包的发送速率和时序。
  unsigned fTimestampPosition;              // 记录时间戳的位置。用于确定在RTP包中存储时间戳的位置。
  unsigned fSpecialHeaderPosition;          // 记录特殊头部的位置。特殊头部是指紧随RTP头部后的特定格式的头部。
  unsigned fSpecialHeaderSize;              // 特殊头部的大小，以字节为单位
  unsigned fCurFrameSpecificHeaderPosition; // 记录当前帧特定头部的位置。帧特定头部是指在RTP包中每个帧之前的特定格式的头部。
  unsigned fCurFrameSpecificHeaderSize;     // 当前帧特定头部的大小，以字节为单位
  unsigned fTotalFrameSpecificHeaderSizes;  // 所有帧特定头部的总大小，以字节为单位。
  unsigned fOurMaxPacketSize;               // 当前RTP包的最大大小，以字节为单位

  onSendErrorFunc *fOnSendErrorFunc; // 指向发送错误回调函数的指针。如果在发送RTP包时发生错误，会调用此回调函数。
  void *fOnSendErrorData;            // 与发送错误回调函数相关的用户数据。可以在回调函数中使用该数据。
};

#endif
