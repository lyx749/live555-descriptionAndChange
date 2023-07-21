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
// A filter that breaks up an MPEG video elementary stream into
//   headers and frames
// C++ header

#ifndef _MPEG_VIDEO_STREAM_FRAMER_HH
#define _MPEG_VIDEO_STREAM_FRAMER_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

class TimeCode
{
public:
  TimeCode();
  virtual ~TimeCode();

  int operator==(TimeCode const &arg2);
  unsigned days, hours, minutes, seconds, pictures;
};

class MPEGVideoStreamFramer : public FramedFilter
{
public:
  // a hack for implementing the RTP 'M' bit
  Boolean &pictureEndMarker() { return fPictureEndMarker; }

  // called if there is a discontinuity (seeking) in the input,在输入流中发生不连续性（如跳转）时调用，用于清空输入缓冲区
  void flushInput();

protected:
  MPEGVideoStreamFramer(UsageEnvironment &env, FramedSource *inputSource);
  // we're an abstract base class
  virtual ~MPEGVideoStreamFramer();
  // sets "fPresentationTime" fPresentationTime 存储了计算得到的展示时间，以秒和微秒表示。该方法根据时间码和帧率等信息，计算出每个图片的正确展示时间。
  void computePresentationTime(unsigned numAdditionalPictures);

  //设置时间码（TimeCode）
  void setTimeCode(unsigned hours, unsigned minutes, unsigned seconds,
                   unsigned pictures, unsigned picturesSinceLastGOP);

protected: // redefined virtual functions

  /// @brief 重新定义的虚函数，用于获取下一帧数据。具体实现由派生类提供。
  virtual void doGetNextFrame();


  /// @brief 重新定义的虚函数，停止获取帧数据的操作。具体实现由派生类提供。
  virtual void doStopGettingFrames();

private:

  /// @brief 重置解析器和相关状态。
  void reset();

  //继续处理读取的数据，调用 continueReadProcessing() 函数。
  static void continueReadProcessing(void *clientData,
                                     unsigned char *ptr, unsigned size,
                                     struct timeval presentationTime);
  void continueReadProcessing();

protected:
  double fFrameRate;                    // Note: For MPEG-4, this is really a 帧率
  unsigned fPictureCount;               // hack used to implement doGetNextFrame()图片计数
  Boolean fPictureEndMarker;            // 图片结束标记
  struct timeval fPresentationTimeBase; // 展示时间基准

  // parsing state
  class MPEGVideoStreamParser *fParser; // 解析器
  friend class MPEGVideoStreamParser;   // hack

private:
  /**
   * 用于存储当前和上一个 GOP（Group of Pictures）的时间码（TimeCode）。
   * GOP 是一组连续的视频帧，具有相同的时间参考。时间码用于指示视频帧在 GOP 中的相对位置。
   */
  TimeCode fCurGOPTimeCode, fPrevGOPTimeCode;
  unsigned fPicturesAdjustment;   // 用于调整图片数目的变量。它是一个无符号整数，表示当前帧的图片数目相对于上一个 GOP 的图片数目的调整值。
  double fPictureTimeBase;        // 用于计算图片的展示时间的基准值。它是一个浮点数，用于确定展示时间的单位。
  unsigned fTcSecsBase;           // 用于计算时间码的秒数基准值。它是一个无符号整数，表示时间码的秒数部分的基准值。
  Boolean fHaveSeenFirstTimeCode; // 用于标记是否已经获取到第一个时间码。它是一个布尔值，用于指示是否已经读取到视频流中的第一个时间码。
};

#endif
