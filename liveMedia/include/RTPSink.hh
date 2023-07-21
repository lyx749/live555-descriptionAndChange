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
// RTP Sinks
// C++ header

#ifndef _RTP_SINK_HH
#define _RTP_SINK_HH

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif
#ifndef _RTP_INTERFACE_HH
#include "RTPInterface.hh"
#endif

class RTPTransmissionStatsDB; // forward

/**
 * 该类提供了一种用于发送RTP数据的接收器，用于将媒体数据通过RTP协议发送到网络中。它具有管理RTP参数、呈现时间、
 * 统计信息等的功能。通过派生该类并实现虚函数，可以实现特定类型的RTP数据包发送器。
 */
class RTPSink : public MediaSink
{
public:
  static Boolean lookupByName(UsageEnvironment &env, char const *sinkName,
                              RTPSink *&resultSink);

  // used by RTSP servers:

  /// @brief 返回组播地址类const函数不可修改
  Groupsock const &groupsockBeingUsed() const { return *(fRTPInterface.gs()); }

  /// @brief 返回组播地址类非const函数可修改
  Groupsock &groupsockBeingUsed() { return *(fRTPInterface.gs()); }

  /// @brief 返回RTP payloadType
  unsigned char rtpPayloadType() const { return fRTPPayloadType; }

  /// @brief 返回RTP时间戳的频率，表示每秒钟的时间戳数
  unsigned rtpTimestampFrequency() const { return fTimestampFrequency; }

  /// @brief 设置RTP时间戳的频率，表示每秒钟的时间戳数
  void setRTPTimestampFrequency(unsigned freq)
  {
    fTimestampFrequency = freq;
  }

  /// @brief 返回rtpPayloadFormatName
  char const *rtpPayloadFormatName() const { return fRTPPayloadFormatName; }

  /// @brief 返回媒体数据通道的数量
  unsigned numChannels() const { return fNumChannels; }

  /// @brief default SDP media (m=) type, unless redefined by subclasses
  virtual char const *sdpMediaType() const; // for use in SDP m= lines

  /// @brief returns a string to be delete[]d
  virtual char *rtpmapLine() const;

  // optional SDP line (e.g. a=fmtp:...) 具体实现在具体子类
  virtual char const *auxSDPLine();

  /// @brief 返回当前RTP数据包序列号
  u_int16_t currentSeqNo() const { return fSeqNo; }

  /// @brief 预设下一个RTP数据包的时间戳（timestamp）并返回该时间戳
  u_int32_t presetNextTimestamp();
  // ensures that the next timestamp to be used will correspond to
  // the current 'wall clock' time.

  /// @brief 返回RTP传输统计数据库（RTPTransmissionStatsDB）的指针，用于存储和管理RTP传输的统计信息。
  RTPTransmissionStatsDB &transmissionStatsDB() const
  {
    return *fTransmissionStatsDB;
  }

  /// @brief 返回下一个RTP时间戳是否已经预设
  Boolean nextTimestampHasBeenPreset() const { return fNextTimestampHasBeenPreset; }

  /// @brief 是否启用发送RTCP
  Boolean &enableRTCPReports() { return fEnableRTCPReports; }

  /// @brief 获取总共发送的字节数(包含头部信息)和总共发送的时间
  void getTotalBitrate(unsigned &outNumBytes, double &outElapsedTime);
  // returns the number of bytes sent since the last time that we
  // were called, and resets the counter.

  /// @brief 返回RTP接收器对象的创建时间
  struct timeval const &creationTime() const { return fCreationTime; }

  /// @brief 返回记录初始展示时间。在实时传输中，用于指示RTP数据包的播放时间
  struct timeval const &initialPresentationTime() const { return fInitialPresentationTime; }

  /// @brief 返回最近接收的RTP数据包的播放时间
  struct timeval const &mostRecentPresentationTime() const { return fMostRecentPresentationTime; }

  /// @brief 将初始的RTP数据包播放时间和最近接收的RTP数据包播放时间两个成员变量重置为0
  void resetPresentationTimes();

  // Hacks to allow sending RTP over TCP (RFC 2236, section 10.12):

  /// @brief 设置流套接字和流通道ID(RTP-Over-TCP)本质上是调用RTPInterface::setStreamSocket
  void setStreamSocket(int sockNum, unsigned char streamChannelId)
  {
    fRTPInterface.setStreamSocket(sockNum, streamChannelId);
  }

  /// @brief 添加流套接字和流通道ID(RTP-Over-TCP)本质上是调用RTPInterface::addStreamSocket
  void addStreamSocket(int sockNum, unsigned char streamChannelId)
  {
    fRTPInterface.addStreamSocket(sockNum, streamChannelId);
  }

  /// @brief 删除流套接字下的流通到id，如果该套接字下所有的流通道id都被删除了，则删除这个套接字(RTP-Over-TCP)本质上是调用RTPInterface::removeStreamSocket
  void removeStreamSocket(int sockNum, unsigned char streamChannelId)
  {
    fRTPInterface.removeStreamSocket(sockNum, streamChannelId);
  }

  /// @brief 返回估计的比特率（以千位/秒为单位），在创建RTP接收器时可以设置，如果未知则为0。
  unsigned &estimatedBitrate() { return fEstimatedBitrate; } // kbps; usually 0 (i.e., unset)

  // later need a means of changing the SSRC if there's a collision #####
  /// @brief 返回RTP数据包的同步信源标识符（SSRC），用于唯一标识发送RTP数据包的源
  u_int32_t SSRC() const { return fSSRC; }

protected:
  RTPSink(UsageEnvironment &env,
          Groupsock *rtpGS, unsigned char rtpPayloadType,
          u_int32_t rtpTimestampFrequency,
          char const *rtpPayloadFormatName,
          unsigned numChannels);
  // abstract base class

  virtual ~RTPSink();

  // used by RTCP:
  friend class RTCPInstance;
  friend class RTPTransmissionStats;

  /// @brief 将tv转换为RTP时间戳
  u_int32_t convertToRTPTimestamp(struct timeval tv);

  /// @brief 返回已发送的RTP数据包的数量。每发送一个RTP数据包，该计数器就会递增
  unsigned packetCount() const { return fPacketCount; }

  /// @brief 返回已发送的RTP数据包的总字节数（不包括RTP头部）。每发送一个RTP数据包，将会增加该数据包的字节数(不包括头部)
  unsigned octetCount() const { return fOctetCount; }

protected:
  RTPInterface fRTPInterface;                 // 用于处理RTP和RTCP数据的发送和接收
  unsigned char fRTPPayloadType;              // 这是一个8位无符号整数，表示RTP数据包的有效负载类型。有效负载类型用于标识RTP数据包携带的媒体数据的类型
  unsigned fPacketCount;                      // 已发送的RTP数据包的数量。每发送一个RTP数据包，该计数器就会递增。
  unsigned fOctetCount;                       // 已发送的RTP数据包的总字节数（不包括RTP头部）。每发送一个RTP数据包，将会增加该数据包的字节数(不包括头部)
  unsigned fTotalOctetCount;                  // 总共已发送的字节数，包括所有RTP数据包的字节数以及RTP头部的字节数。
  struct timeval fTotalOctetCountStartTime;   // 用于记录总字节数计数的起始时间。通过在不同时间点记录总字节数，可以计算发送速率等统计信息。
  struct timeval fInitialPresentationTime;    // 用于记录初始展示时间。在实时传输中，用于指示RTP数据包的播放时间
  struct timeval fMostRecentPresentationTime; // 用于指示最近接收的RTP数据包的播放时间
  u_int32_t fCurrentTimestamp;                // 表示当前的RTP时间戳。时间戳是RTP数据包中用于同步和定时的重要字段。
  u_int16_t fSeqNo;                           // 表示当前的RTP序列号。序列号是RTP数据包中用于标识数据包顺序的字段。每发送一个RTP数据包，该序列号会递增。

private:
  // redefined virtual functions:
  virtual Boolean isRTPSink() const;

private:
  u_int32_t fSSRC;                     // RTP数据包的同步信源标识符（SSRC），用于唯一标识发送RTP数据包的源
  u_int32_t fTimestampBase;            // RTP时间戳的基准值，用于计算和生成RTP时间戳
  unsigned fTimestampFrequency;        // RTP时间戳的频率，表示每秒钟的时间戳数
  Boolean fNextTimestampHasBeenPreset; // 标志位，表示下一个RTP时间戳是否已经预设。
  Boolean fEnableRTCPReports;          // 标志位，表示是否启用发送RTCP（Real-time Transport Control Protocol）的"SR"（Sender Report）报告。默认值为True
  char const *fRTPPayloadFormatName;   // RTP数据包的有效负载格式名称，描述了数据包中有效负载的格式。
  unsigned fNumChannels;               // 媒体数据的通道数
  struct timeval fCreationTime;        // RTP接收器对象的创建时间
  unsigned fEstimatedBitrate;          // 估计的比特率（以千位/秒为单位），在创建RTP接收器时可以设置，如果未知则为0。

  RTPTransmissionStatsDB *fTransmissionStatsDB; // RTP传输统计数据库（RTPTransmissionStatsDB）的指针，用于存储和管理RTP传输的统计信息。
};

class RTPTransmissionStats; // forward

/// @brief 它在RTPSink类中起到了重要的作用，用于跟踪和记录RTP传输的性能指标和统计数据，用来实现RTCP的RR报文(类似于RTPTransmissionStats管理器)
class RTPTransmissionStatsDB
{
public:
  /// @brief 返回接收者数量
  unsigned numReceivers() const { return fNumReceivers; }

  class Iterator
  {
  public:
    Iterator(RTPTransmissionStatsDB &receptionStatsDB);
    virtual ~Iterator();

    RTPTransmissionStats *next();
    // NULL if none

  private:
    HashTable::Iterator *fIter;
  };

  // RTCP（Real-time Transport Control Protocol）的RR（Receiver Report）报文是RTCP报文中的一种类型。它主要用于接收者向发送者发送反馈信息，提供了有关媒体传输质量的统计数据。RR报文是由接收者发送的，
  // 用于告知发送者有关其接收到的媒体数据的情况。
  // RR报文中包含以下信息：
  // SSRC（同步源标识符）：指定发送者的SSRC标识符，用于告知发送者哪个接收者发送了该报文。
  // 丢包统计：包含丢失的RTP数据包数量和序列号，用于指示在传输过程中丢失了多少个数据包。
  // 抖动（Jitter）：指示接收者接收到的数据包的传输时间的变化，用于测量网络抖动的程度。
  // 延迟（Last SR Time）：指示接收者上一次接收到发送者发送的SR（Sender Report）报文的时间戳。
  // 与SR报文时间的差值（Diff SR-RR Time）：指示接收者与发送者的SR报文时间戳之间的差值，用于计算接收者的延迟和抖动。
  // 通过RR报文，发送者可以了解到接收者的接收情况和网络传输状况，以便进行调整和优化媒体传输过程，从而提供更好的实时传输质量。RR报文是实时多媒体通信中重要的反馈机制，帮助保证数据传输的稳定和可靠性。
  // The following is called whenever a RTCP RR packet is received:
  void noteIncomingRR(u_int32_t SSRC, struct sockaddr_in const &lastFromAddress,
                      unsigned lossStats, unsigned lastPacketNumReceived,
                      unsigned jitter, unsigned lastSRTime, unsigned diffSR_RRTime);

  // RTCP（Real-time Transport Control Protocol）的BYE（Goodbye）报文是RTCP报文中的一种类型。它主要用于通知参与会话的其他参与者，该参与者即将离开会话，并结束其参与者角色。
  // BYE报文包含以下信息：
  // SSRC（同步源标识符）：指定发送BYE报文的参与者的SSRC标识符，用于告知其他参与者是哪个参与者发出的离开通知。
  // CNAME（Canonical Name）：参与者的规范名字，用于标识参与者。CNAME是会话中所有参与者唯一的标识符。
  // 附加数据（Optional Reason for Leaving）：BYE报文可以携带可选的附加数据，用于说明参与者离开的原因，比如用户主动退出、网络问题等。
  // 当一个参与者决定离开会话时，它会发送一个BYE报文给其他参与者，其他参与者收到BYE报文后，会知道该参与者已经离开了会话。接收到BYE报文的参与者可能会根据情况做出相应的处理，比如更新会话状态、停止传输媒体数据等。
  // The following is called when a RTCP BYE packet is received:
  void removeRecord(u_int32_t SSRC);

  RTPTransmissionStats *lookup(u_int32_t SSRC) const;

private: // constructor and destructor, called only by RTPSink:
  friend class RTPSink;
  RTPTransmissionStatsDB(RTPSink &rtpSink);
  virtual ~RTPTransmissionStatsDB();

private:
  void add(u_int32_t SSRC, RTPTransmissionStats *stats);

private:
  friend class Iterator;
  unsigned fNumReceivers; // 接收者数量
  RTPSink &fOurRTPSink;   // RTPSink对象的引用
  HashTable *fTable;      // 哈希表，用于存储和管理SSRC与RTP传输统计数据之间的映射关系。
};

/// @brief 该类用于跟踪RTP传输统计信息，记录有关RTP传输的相关数据，如数据包的接收情况、丢包情况、抖动等
class RTPTransmissionStats
{
public:
  /// @brief 返回统计信息对应的SSRC标识符
  u_int32_t SSRC() const { return fSSRC; }

  /// @brief 返回上次接收到RTP数据包的地址信息
  struct sockaddr_in const &lastFromAddress() const { return fLastFromAddress; }

  /// @brief 返回上次接收到RTP数据包的地址信息
  unsigned lastPacketNumReceived() const { return fLastPacketNumReceived; }

  /// @brief 返回首个报告的RTP数据包序列号
  unsigned firstPacketNumReported() const { return fFirstPacketNumReported; }

  /// @brief 返回自创建以来丢失的总数据包数量
  unsigned totNumPacketsLost() const { return fTotNumPacketsLost; }

  /// @brief 返回数据包抖动值，用于衡量数据包的传输延迟变化情况
  unsigned jitter() const { return fJitter; }

  /// @brief 返回上次收到的SR（发送者报告）报文时间戳
  unsigned lastSRTime() const { return fLastSRTime; }

  /// @brief 返回SR报文和RR（接收者报告）报文之间的时间差
  unsigned diffSR_RRTime() const { return fDiffSR_RRTime; }

  // The round-trip delay (in units of 1/65536 seconds) computed from
  // the most recently-received RTCP RR packet.
  /// @brief 计算最近接收到的RTCP RR（Receiver Report）报文所指示的往返时延
  unsigned roundTripDelay() const;

  /// @brief 返回信息创建的时间戳
  struct timeval const &timeCreated() const { return fTimeCreated; }

  /// @brief 返回最近接收到RTP数据包的时间戳
  struct timeval const &lastTimeReceived() const { return fTimeReceived; }

  /// @brief 获取总字节数计数，以64位整数的高位和低位返回。
  void getTotalOctetCount(u_int32_t &hi, u_int32_t &lo);

  /// @brief 获取总数据包计数，以64位整数的高位和低位返回。
  void getTotalPacketCount(u_int32_t &hi, u_int32_t &lo);

  // Information which requires at least two RRs to have been received:
  /// @brief 返回自上次接收RR报文以来接收的RTP数据包数量。
  unsigned packetsReceivedSinceLastRR() const;

  /// @brief 返回丢包率，以8位定点数表示。
  u_int8_t packetLossRatio() const { return fPacketLossRatio; }

  // as an 8-bit fixed-point number
  /// @brief  返回两个RR报文之间的丢包数量
  int packetsLostBetweenRR() const;

private:
  // called only by RTPTransmissionStatsDB:
  friend class RTPTransmissionStatsDB;
  RTPTransmissionStats(RTPSink &rtpSink, u_int32_t SSRC);
  virtual ~RTPTransmissionStats();

  void noteIncomingRR(struct sockaddr_in const &lastFromAddress,
                      unsigned lossStats, unsigned lastPacketNumReceived,
                      unsigned jitter,
                      unsigned lastSRTime, unsigned diffSR_RRTime);

private:
  RTPSink &fOurRTPSink;                                 // 对应的RTPSink对象的引用
  u_int32_t fSSRC;                                      // 统计信息对应的SSRC标识符
  struct sockaddr_in fLastFromAddress;                  // 上次接收到RTP数据包的地址信息
  unsigned fLastPacketNumReceived;                      // 上次接收到的RTP数据包序列号
  u_int8_t fPacketLossRatio;                            // 丢包率，以8位定点数表示
  unsigned fTotNumPacketsLost;                          // 自创建以来丢失的总数据包数量
  unsigned fJitter;                                     // 数据包抖动值，用于衡量数据包的传输延迟变化情况
  unsigned fLastSRTime;                                 // 上次收到的SR（发送者报告）报文时间戳
  unsigned fDiffSR_RRTime;                              // SR报文和RR（接收者报告）报文之间的时间差
  struct timeval fTimeCreated;                          // 统计信息创建的时间戳
  struct timeval fTimeReceived;                         // 最近接收到RTP数据包的时间戳
  Boolean fAtLeastTwoRRsHaveBeenReceived;               // 标志位，表示是否至少收到两个RR报文
  unsigned fOldLastPacketNumReceived;                   // 上一次记录的RTP数据包序列号
  unsigned fOldTotNumPacketsLost;                       // 上一次记录的丢失的总数据包数量
  Boolean fFirstPacket;                                 // 标志位，表示是否为第一个报告的RTP数据包
  unsigned fFirstPacketNumReported;                     // 首个报告的RTP数据包序列号
  u_int32_t fLastOctetCount;                            //  用于记录上次记录的字节计数值。字节计数表示自创建以来传输的总字节数。每次收到新的RTP数据包时，会更新这个值。
  u_int32_t fTotalOctetCount_hi, fTotalOctetCount_lo;   // 用于记录总字节计数值。由于字节计数可能非常大，采用64位整数表示。这两个变量表示一个64位整数的高32位和低32位部分。通过将这两部分合并，可以得到完整的64位字节计数值。
  u_int32_t fLastPacketCount;                           // 用于记录上次记录的数据包计数值。数据包计数表示自创建以来传输的总数据包数量。每次收到新的RTP数据包时，会更新这个值
  u_int32_t fTotalPacketCount_hi, fTotalPacketCount_lo; // 用于记录总数据包计数值。由于数据包计数可能非常大，采用64位整数表示。这两个变量表示一个64位整数的高32位和低32位部分。通过将这两部分合并，可以得到完整的64位数据包计数值。
};

#endif
