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
// RTP sink for H.264 or H.265 video
// Implementation

#include "H264or5VideoRTPSink.hh"
#include "H264or5VideoStreamFramer.hh"

////////// H264or5Fragmenter definition //////////

// Because of the ideosyncracies of the H.264 RTP payload format, we implement
// "H264or5VideoRTPSink" using a separate "H264or5Fragmenter" class that delivers,
// to the "H264or5VideoRTPSink", only fragments that will fit within an outgoing
// RTP packet.  I.e., we implement fragmentation in this separate "H264or5Fragmenter"
// class, rather than in "H264or5VideoRTPSink".
// (Note: This class should be used only by "H264or5VideoRTPSink", or a subclass.)

class H264or5Fragmenter : public FramedFilter
{
public:
  H264or5Fragmenter(int hNumber, UsageEnvironment &env, FramedSource *inputSource,
                    unsigned inputBufferMax, unsigned maxOutputPacketSize);
  virtual ~H264or5Fragmenter();

  // 表示最后一个分片是否完成了当前NAL单元（Network Abstraction Layer Unit）。
  Boolean lastFragmentCompletedNALUnit() const { return fLastFragmentCompletedNALUnit; }

private: // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

private:
  static void afterGettingFrame(void *clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);
  void reset();

private:
  int fHNumber;                          // 表示是H.264还是H.265（HEVC）编码。如果是H.264编码，值为264，如果是H.265（HEVC）编码，值为265。
  unsigned fInputBufferSize;             // 输入缓冲区大小
  unsigned fMaxOutputPacketSize;         // 输出包的最大大小
  unsigned char *fInputBuffer;           // 指向输入缓冲区的指针。
  unsigned fNumValidDataBytes;           // 有效的数据字节数。
  unsigned fCurDataOffset;               // 当前数据的偏移量。
  unsigned fSaveNumTruncatedBytes;       // 保存截断的字节数。
  Boolean fLastFragmentCompletedNALUnit; // 布尔值，表示最后一个分片是否完成了当前NAL单元。
};

////////// H264or5VideoRTPSink implementation //////////

H264or5VideoRTPSink ::H264or5VideoRTPSink(int hNumber,
                                          UsageEnvironment &env, Groupsock *RTPgs, unsigned char rtpPayloadFormat,
                                          u_int8_t const *vps, unsigned vpsSize,
                                          u_int8_t const *sps, unsigned spsSize,
                                          u_int8_t const *pps, unsigned ppsSize)
    : VideoRTPSink(env, RTPgs, rtpPayloadFormat, 90000, hNumber == 264 ? "H264" : "H265"),
      fHNumber(hNumber), fOurFragmenter(NULL), fFmtpSDPLine(NULL)
{
  if (vps != NULL)
  {
    fVPSSize = vpsSize;
    fVPS = new u_int8_t[fVPSSize];
    memmove(fVPS, vps, fVPSSize);
  }
  else
  {
    fVPSSize = 0;
    fVPS = NULL;
  }
  if (sps != NULL)
  {
    fSPSSize = spsSize;
    fSPS = new u_int8_t[fSPSSize];
    memmove(fSPS, sps, fSPSSize);
  }
  else
  {
    fSPSSize = 0;
    fSPS = NULL;
  }
  if (pps != NULL)
  {
    fPPSSize = ppsSize;
    fPPS = new u_int8_t[fPPSSize];
    memmove(fPPS, pps, fPPSSize);
  }
  else
  {
    fPPSSize = 0;
    fPPS = NULL;
  }
}

H264or5VideoRTPSink::~H264or5VideoRTPSink()
{
  fSource = fOurFragmenter; // hack: in case "fSource" had gotten set to NULL before we were called
  delete[] fFmtpSDPLine;
  delete[] fVPS;
  delete[] fSPS;
  delete[] fPPS;
  stopPlaying(); // call this now, because we won't have our 'fragmenter' when the base class destructor calls it later.

  // Close our 'fragmenter' as well:
  Medium::close(fOurFragmenter);
  fSource = NULL; // for the base class destructor, which gets called next
}

Boolean H264or5VideoRTPSink::continuePlaying()
{
  // First, check whether we have a 'fragmenter' class set up yet.
  // If not, create it now:
  if (fOurFragmenter == NULL)
  {
    fOurFragmenter = new H264or5Fragmenter(fHNumber, envir(), fSource, OutPacketBuffer::maxSize,
                                           ourMaxPacketSize() - 12 /*RTP hdr size*/);
  }
  else
  {
    fOurFragmenter->reassignInputSource(fSource);
  }
  fSource = fOurFragmenter;

  // Then call the parent class's implementation:
  return MultiFramedRTPSink::continuePlaying();
}

void H264or5VideoRTPSink::doSpecialFrameHandling(unsigned /*fragmentationOffset*/,
                                                 unsigned char * /*frameStart*/,
                                                 unsigned /*numBytesInFrame*/,
                                                 struct timeval framePresentationTime,
                                                 unsigned /*numRemainingBytes*/)
{
  // Set the RTP 'M' (marker) bit iff
  // 1/ The most recently delivered fragment was the end of (or the only fragment of) an NAL unit, and
  // 2/ This NAL unit was the last NAL unit of an 'access unit' (i.e. video frame).
  if (fOurFragmenter != NULL)
  {
    H264or5VideoStreamFramer *framerSource = (H264or5VideoStreamFramer *)(fOurFragmenter->inputSource());
    // This relies on our fragmenter's source being a "H264or5VideoStreamFramer".
    if (((H264or5Fragmenter *)fOurFragmenter)->lastFragmentCompletedNALUnit() && framerSource != NULL && framerSource->pictureEndMarker())
    {
      // 通过检查当前对象的fragmenter是否为空（fOurFragmenter != NULL）来判断是否需要设置RTP标记位。如果fragmenter为空，则不执行后续操作。
      // 获取fragmenter的源数据源（H264or5VideoStreamFramer），这里假设fragmenter的源数据源是H264or5VideoStreamFramer类型。
      // 检查最近一个分片是否完成了NAL单元（((H264or5Fragmenter *)fOurFragmenter)->lastFragmentCompletedNALUnit()）。如果最近一个分片完成了NAL单元，并且数据源不为空，并且数据源中标志了视频帧结束（framerSource->pictureEndMarker()），则满足设置RTP标记位的条件。
      // 如果满足设置RTP标记位的条件，则调用setMarkerBit()函数设置RTP 'M'（标记）位，并将数据源的视频帧结束标志设置为False（framerSource->pictureEndMarker() = False）。
      // 最后，通过调用setTimestamp(framePresentationTime)函数设置RTP包的时间戳，将视频帧的呈现时间传递给RTP包。
      setMarkerBit();
      framerSource->pictureEndMarker() = False;
    }
  }

  setTimestamp(framePresentationTime);
}

Boolean H264or5VideoRTPSink ::frameCanAppearAfterPacketStart(unsigned char const * /*frameStart*/,
                                                             unsigned /*numBytesInFrame*/) const
{
  return False;
}

////////// H264or5Fragmenter implementation //////////

H264or5Fragmenter::H264or5Fragmenter(int hNumber,
                                     UsageEnvironment &env, FramedSource *inputSource,
                                     unsigned inputBufferMax, unsigned maxOutputPacketSize)
    : FramedFilter(env, inputSource),
      fHNumber(hNumber),
      fInputBufferSize(inputBufferMax + 1), fMaxOutputPacketSize(maxOutputPacketSize)
{
  fInputBuffer = new unsigned char[fInputBufferSize];
  reset();
}

H264or5Fragmenter::~H264or5Fragmenter()
{
  delete[] fInputBuffer;
  detachInputSource(); // so that the subsequent ~FramedFilter() doesn't delete it
}

void H264or5Fragmenter::doGetNextFrame()
{
  if (fNumValidDataBytes == 1)
  {
    // We have no NAL unit data currently in the buffer.  Read a new one:
    //  注意：fInputBuffer[0]存储的是保留的起始标志字节
    // 这里调用的是该类父类FramedFiler内部包含的FramedFileSource下的基类的DoGetNextFrame，并且通过设置其回调函数为该类的成员静态函数afterGettingFrame，从而使其能够从源将数据读取到
    // fInputBuffer中，然后还能返回到这继续执行
    fInputSource->getNextFrame(&fInputBuffer[1], fInputBufferSize - 1,
                               afterGettingFrame, this,
                               FramedSource::handleClosure, this);
  }
  else
  {
    // We have NAL unit data in the buffer.  There are three cases to consider:
    // 1. There is a new NAL unit in the buffer, and it's small enough to deliver
    //    to the RTP sink (as is).
    // 2. There is a new NAL unit in the buffer, but it's too large to deliver to
    //    the RTP sink in its entirety.  Deliver the first fragment of this data,
    //    as a FU packet, with one extra preceding header byte (for the "FU header").
    // 3. There is a NAL unit in the buffer, and we've already delivered some
    //    fragment(s) of this.  Deliver the next fragment of this data,
    //    as a FU packet, with two (H.264) or three (H.265) extra preceding header bytes
    //    (for the "NAL header" and the "FU header").

    // 如果缓冲区中有NAL单元数据。有三种情况：
    // 1. 缓冲区中有一个新的NAL单元，并且它小于等于RTP包的最大大小，可以直接传输
    // 2. 缓冲区中有一个新的NAL单元，但它太大无法一次传输。传输第一个片段作为FU包，并添加一个额外的前导字节（FU头）。
    // 3. 缓冲区中有一个NAL单元，之前已经传输了一些片段。传输下一个片段作为FU包，并添加两个（H.264）或三个（H.265）额外的前导字节（NAL头和FU头）。

    if (fMaxSize < fMaxOutputPacketSize)
    { // shouldn't happen
      envir() << "H264or5Fragmenter::doGetNextFrame(): fMaxSize ("
              << fMaxSize << ") is smaller than expected\n";
    }
    else
    {
      fMaxSize = fMaxOutputPacketSize;
    }

    // 默认情况下，假设已完成最后一个NAL单元的片段传输
    fLastFragmentCompletedNALUnit = True; // by default

    // fCurDataOffset等于1表示当前缓冲区中有一个新的NAL单元数据
    if (fCurDataOffset == 1)
    { // case 1 or 2
      if (fNumValidDataBytes - 1 <= fMaxSize)
      { // 情况1：缓冲区中的NAL单元数据小于等于RTP包的最大大小，可以直接传输
        memmove(fTo, &fInputBuffer[1], fNumValidDataBytes - 1);
        fFrameSize = fNumValidDataBytes - 1;
        fCurDataOffset = fNumValidDataBytes;
      }
      else
      { // case 2
        // We need to send the NAL unit data as FU packets.  Deliver the first
        // packet now.  Note that we add "NAL header" and "FU header" bytes to the front
        // of the packet (overwriting the existing "NAL header").
        // 情况2：缓冲区中的NAL单元数据太大，需要将其作为FU包传输。传输第一个片段，并添加额外的前导字节（FU头）。
        // 根据H.264或H.265设置FU头的内容
        if (fHNumber == 264)
        {
          fInputBuffer[0] = (fInputBuffer[1] & 0xE0) | 28;   // FU indicator  FU指示符
          fInputBuffer[1] = 0x80 | (fInputBuffer[1] & 0x1F); // FU header (with S bit)  FU头（带有S位）
        }
        else
        { // 265
          u_int8_t nal_unit_type = (fInputBuffer[1] & 0x7E) >> 1;
          fInputBuffer[0] = (fInputBuffer[1] & 0x81) | (49 << 1); // Payload header (1st byte)  负载头（第一个字节）
          fInputBuffer[1] = fInputBuffer[2];                      // Payload header (2nd byte)  负载头（第二个字节）
          fInputBuffer[2] = 0x80 | nal_unit_type;                 // FU header (with S bit)     FU头（带有S位）
        }
        // 将第一个片段传输到输出缓冲区fTo，并记录片段大小fFrameSize
        memmove(fTo, fInputBuffer, fMaxSize);
        fFrameSize = fMaxSize;
        fCurDataOffset += fMaxSize - 1;
        fLastFragmentCompletedNALUnit = False;
      }
    }
    else
    { // case 3
      // We are sending this NAL unit data as FU packets.  We've already sent the
      // first packet (fragment).  Now, send the next fragment.  Note that we add
      // "NAL header" and "FU header" bytes to the front.  (We reuse these bytes that
      // we already sent for the first fragment, but clear the S bit, and add the E
      // bit if this is the last fragment.)
      unsigned numExtraHeaderBytes;
      if (fHNumber == 264)
      {
        fInputBuffer[fCurDataOffset - 2] = fInputBuffer[0];         // FU indicator
        fInputBuffer[fCurDataOffset - 1] = fInputBuffer[1] & ~0x80; // FU header (no S bit)
        numExtraHeaderBytes = 2;
      }
      else
      {                                                             // 265
        fInputBuffer[fCurDataOffset - 3] = fInputBuffer[0];         // Payload header (1st byte)
        fInputBuffer[fCurDataOffset - 2] = fInputBuffer[1];         // Payload header (2nd byte)
        fInputBuffer[fCurDataOffset - 1] = fInputBuffer[2] & ~0x80; // FU header (no S bit)
        numExtraHeaderBytes = 3;
      }
      unsigned numBytesToSend = numExtraHeaderBytes + (fNumValidDataBytes - fCurDataOffset);
      if (numBytesToSend > fMaxSize)
      {
        // We can't send all of the remaining data this time:
        numBytesToSend = fMaxSize;
        fLastFragmentCompletedNALUnit = False;
      }
      else
      {
        // This is the last fragment:
        fInputBuffer[fCurDataOffset - 1] |= 0x40; // set the E bit in the FU header
        fNumTruncatedBytes = fSaveNumTruncatedBytes;
      }
      memmove(fTo, &fInputBuffer[fCurDataOffset - numExtraHeaderBytes], numBytesToSend);
      fFrameSize = numBytesToSend;
      fCurDataOffset += numBytesToSend - numExtraHeaderBytes;
    }

    if (fCurDataOffset >= fNumValidDataBytes)
    {
      // We're done with this data.  Reset the pointers for receiving new data:
      fNumValidDataBytes = fCurDataOffset = 1;
    }

    // Complete delivery to the client:
    FramedSource::afterGetting(this);
  }
}

void H264or5Fragmenter::doStopGettingFrames()
{
  // Make sure that we don't have any stale data fragments lying around, should we later resume:
  reset();
  FramedFilter::doStopGettingFrames();
}

void H264or5Fragmenter::afterGettingFrame(void *clientData, unsigned frameSize,
                                          unsigned numTruncatedBytes,
                                          struct timeval presentationTime,
                                          unsigned durationInMicroseconds)
{
  H264or5Fragmenter *fragmenter = (H264or5Fragmenter *)clientData;
  fragmenter->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime,
                                 durationInMicroseconds);
}

void H264or5Fragmenter::afterGettingFrame1(unsigned frameSize,
                                           unsigned numTruncatedBytes,
                                           struct timeval presentationTime,
                                           unsigned durationInMicroseconds)
{
  fNumValidDataBytes += frameSize;
  fSaveNumTruncatedBytes = numTruncatedBytes;
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;

  // Deliver data to the client:
  doGetNextFrame();
}

void H264or5Fragmenter::reset()
{
  fNumValidDataBytes = fCurDataOffset = 1;
  fSaveNumTruncatedBytes = 0;
  fLastFragmentCompletedNALUnit = True;
}
