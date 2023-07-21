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
// A file source that is a plain byte stream (rather than frames)
// C++ header

#ifndef _BYTE_STREAM_FILE_SOURCE_HH
#define _BYTE_STREAM_FILE_SOURCE_HH

#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

/**
 * 类用于表示字节流文件源，并提供了相关的功能和操作，包括创建对象、文件定位、获取帧数据、
 * 停止获取帧数据等。通过这些功能，可以读取文件的数据，并进行适当的处理和流媒体传输配置。
 */
class ByteStreamFileSource : public FramedFileSource
{
public:
  /**
   * preferredFrameSize：该参数表示帧的首选大小。帧是数据流中的一个小单元，
   * 而帧的大小指的是该单元中包含的字节数。设置首选帧大小可以用于指定在传输数据时希望每个帧的大小是多少。
   * 如果值为0，则表示没有首选的帧大小，即没有特定的要求，将根据实际情况自动确定帧的大小。
   * playTimePerFrame：该参数表示每个帧的播放时间。它用于指定每个帧在播放时所需的时间长度。
   * 播放时间可以根据实际需求进行配置，以确保在流媒体传输中每个帧按照期望的时间间隔进行播放。如果值为0，
   * 则表示没有特定的播放时间，播放时间将根据实际情况自动确定。
   */
  /// @brief 静态函数，封装了该类的构造函数，用于创建该类
  /// @param env 用户基础环境变量
  /// @param fileName 文件名
  /// @param preferredFrameSize 表示帧的首选大小
  /// @param playTimePerFrame 表示每个帧的播放时间
  /// @return 创建该类的的类指针
  static ByteStreamFileSource *createNew(UsageEnvironment &env,
                                         char const *fileName,
                                         unsigned preferredFrameSize = 0,
                                         unsigned playTimePerFrame = 0);

  // an alternative version of "createNew()" that's used if you already have
  // an open file.
  static ByteStreamFileSource *createNew(UsageEnvironment &env,
                                         FILE *fid,
                                         unsigned preferredFrameSize = 0,
                                         unsigned playTimePerFrame = 0);

  // 0 means zero-length, unbounded, or unknown
  u_int64_t fileSize() const { return fFileSize; }

  /// @brief 将该文件流指针定位到byteNumber位置(从文件指针开始的位置)
  /// @param byteNumber 需要定位的位置
  /// @param numBytesToStream if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
  void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0);
  
  
  /// @brief 将该文件流指针从当前位置向后偏移offset个字节
  /// @param offset 需要偏移的偏移量
  /// @param numBytesToStream if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
  void seekToByteRelative(int64_t offset, u_int64_t numBytesToStream = 0);

  /// @brief to force EOF handling on the next read
  void seekToEnd(); 

protected:
  ByteStreamFileSource(UsageEnvironment &env,
                       FILE *fid,
                       unsigned preferredFrameSize,
                       unsigned playTimePerFrame);
  // called only by createNew()

  virtual ~ByteStreamFileSource();

  /// @brief 静态函数，本质是对doReadFromFile()的一个封装，先判断还需要读数据吗，如果需要则取调用doReadFromFile
  /// @param source 该类的类指针，用于调用doReadFromFile()
  /// @param mask 文件掩码
  static void fileReadableHandler(ByteStreamFileSource *source, int mask);


  /// @brief 从文件中读取内容，并设置播放时间
  void doReadFromFile();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

protected:
  u_int64_t fFileSize; // 文件的大小

private:
  unsigned fPreferredFrameSize;   // 首选帧大小
  unsigned fPlayTimePerFrame;     // 每帧的播放时间
  Boolean fFidIsSeekable;         // 文件指针是否可寻址
  unsigned fLastPlayTime;         // 上一帧的播放时间
  Boolean fHaveStartedReading;    // 标识是否已开始读取文件
  Boolean fLimitNumBytesToStream; // 标识是否限制要流式传输的字节数
  u_int64_t fNumBytesToStream;    // 要流式传输的字节数（仅在 fLimitNumBytesToStream 为真时使用）
};

#endif
