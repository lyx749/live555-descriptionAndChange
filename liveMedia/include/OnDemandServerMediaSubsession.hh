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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand.
// C++ header

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#define _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH

#ifndef _SERVER_MEDIA_SESSION_HH
#include "ServerMediaSession.hh"
#endif
#ifndef _RTP_SINK_HH
#include "RTPSink.hh"
#endif
#ifndef _BASIC_UDP_SINK_HH
#include "BasicUDPSink.hh"
#endif
#ifndef _RTCP_HH
#include "RTCP.hh"
#endif

/// @brief 用于实现基于点播的流媒体服务器功能。
class OnDemandServerMediaSubsession : public ServerMediaSubsession
{
protected: // we're a virtual base class
  OnDemandServerMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource,
                                portNumBits initialPortNum = 6970,
                                Boolean multiplexRTCPWithRTP = False);
  virtual ~OnDemandServerMediaSubsession();

protected: // redefined virtual functions
  /// @brief 返回SDP描述中媒体子会话的所有行。子类可以通过重定义这个函数来提供特定的SDP描述。
  virtual char const *sdpLines();

  /// @brief 主要就是通过下面的参数获取一个StreamState通过streamToken返回
  /// @param clientSessionId 客户端会话ID
  /// @param clientAddress 客户端地址，网络字节序
  /// @param clientRTPPort 客户端接收RTP的 port
  /// @param clientRTCPPort 客户端接收RTCP的 port
  /// @param tcpSocketNum tcp连接套接字
  /// @param rtpChannelId RTP管道id
  /// @param rtcpChannelId rtcp管道id
  /// @param destinationAddress 目的地址
  /// @param destinationTTL 到目的地址的TTL
  /// @param isMulticast 是否为多播
  /// @param serverRTPPort 服务器的RTP监听端口
  /// @param serverRTCPPort 服务器的RTCP监听端口
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  virtual void getStreamParameters(unsigned clientSessionId,
                                   netAddressBits clientAddress,
                                   Port const &clientRTPPort,
                                   Port const &clientRTCPPort,
                                   int tcpSocketNum,
                                   unsigned char rtpChannelId,
                                   unsigned char rtcpChannelId,
                                   netAddressBits &destinationAddress,
                                   u_int8_t &destinationTTL,
                                   Boolean &isMulticast,
                                   Port &serverRTPPort,
                                   Port &serverRTCPPort,
                                   void *&streamToken);

  // 服务器请求替代字节（Alternative Byte）是一种RTSP（Real-Time Streaming Protocol）中的请求，用于通知服务器在传输过程中使用替代字节
  // 来发送媒体数据。
  // 在某些情况下，客户端可能需要在媒体流的特定位置开始接收数据，而不是从媒体流的起始位置开始。这可能是因为客户端中断了先前的媒体流传输，
  // 然后重新连接并希望从中断位置继续接收数据。为了实现这一点，客户端可以向服务器发送一个"服务器请求替代字节"的RTSP请求。
  // 这个请求使用的RTSP方法是"SETUP"方法，并在请求中包含Range头字段。Range头字段指定了客户端希望从媒体流中接收数据的位置范围。
  // 服务器收到这个请求后，将根据客户端指定的范围进行调整，并开始在该位置处发送媒体数据。
  // 使用替代字节的请求允许客户端实现时间跳转（Seeking）功能，以便从媒体流中的任意位置开始播放。
  // 这对于实现快进、快退、跳过广告或进行回放等功能非常有用。
  // 需要注意的是，支持"服务器请求替代字节"功能的服务器和客户端都必须遵循相应的RTSP标准，
  // 并正确解析和处理"SETUP"请求中的Range头字段。不是所有的媒体流和服务器都支持这个功能，所以在使用时需要确认服务器和客户端都支持"Alternative Byte"功能。

  /// @brief 在客户端请求播放媒体流时，启动流传输。子类可以通过重定义这个函数来处理特定的启动逻辑。
  /// @param clientSessionId 客户端会话标识符
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  /// @param rtcpRRHandler 指向一个用于处理RTCP接收报告(RR)的处理函数指针
  /// @param rtcpRRHandlerClientData 上面那个函数指针的参数
  /// @param rtpSeqNum 用于存储rtpSeqNum的序列号
  /// @param rtpTimestamp 用于存储的rtp时间戳
  /// @param serverRequestAlternativeByteHandler  指向处理服务器请求替代字节（Alternative Byte）的处理程序函数的指针。这个处理程序函数会在服务器接收到客户端的替代字节请求时被调用，以便处理该请求。
  /// @param serverRequestAlternativeByteHandlerClientData 一个指向替代字节处理程序的客户端数据的指针。这个指针可以用于传递额外的信息给替代字节处理程序函数
  virtual void startStream(unsigned clientSessionId, void *streamToken,
                           TaskFunc *rtcpRRHandler,
                           void *rtcpRRHandlerClientData,
                           unsigned short &rtpSeqNum,
                           unsigned &rtpTimestamp,
                           ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
                           void *serverRequestAlternativeByteHandlerClientData);

  /// @brief 暂停流传输。子类可以通过重定义这个函数来处理特定的暂停逻辑。
  /// @param clientSessionId 客户端会话标识符
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  virtual void pauseStream(unsigned clientSessionId, void *streamToken);

  /// @brief 根据客户端的请求，将媒体流定位到指定的时间点（seekNPT），并返回实际定位后的时间点和数据量。
  /// @param clientSessionId 客户端会话标识符
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  /// @param seekNPT 输入参数和输出参数。作为输入参数时，它表示客户端希望从媒体流的哪个时间点进行定位。作为输出参数时，它表示实际定位后的时间点。时间点以秒为单位表示。
  /// @param streamDuration 表示媒体流的总时长。这是一个只读参数，用于告知媒体流的总时长。在进行定位时，客户端可能需要参考媒体流的总时长。
  /// @param numBytes 输出参数。表示从定位点开始的数据量，以字节为单位。在进行定位时，可能需要返回从定位点开始的数据量。
  virtual void seekStream(unsigned clientSessionId, void *streamToken, double &seekNPT, double streamDuration, u_int64_t &numBytes);

  /// @brief 这个函数支持以绝对时间（Absolute time）进行定位。
  /// @param clientSessionId 客户端会话标识符
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  /// @param absStart 输出参数。是一个字符指针的引用，表示客户端希望从媒体流的哪个绝对时间点开始定位。这是一个输出参数，因为定位操作可能会对这个指针进行修改，从而返回实际定位后的绝对时间点
  /// @param absEnd 输出参数。是一个字符指针的引用，表示客户端希望定位到媒体流的哪个绝对时间点结束。这是一个输出参数，因为定位操作可能会对这个指针进行修改，从而返回实际定位后的绝对时间点
  virtual void seekStream(unsigned clientSessionId, void *streamToken, char *&absStart, char *&absEnd);

  // 在实现 nullSeekStream 函数时，通常会忽略客户端的定位请求，直接将媒体流定位到其结束时间点。
  // 这个函数通常用于处理客户端发送的无效定位请求，
  // 或者在某些情况下当无法满足客户端的定位需求时，可以使用该函数来通知客户端定位操作无效。

  /// @brief 用于在媒体流中进行无效（Null）定位操作
  /// @param clientSessionId 客户端会话标识符
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  /// @param streamEndTime  表示客户端希望从媒体流的哪个绝对时间点开始定位。这个时间点通常是媒体流的结束时间点。
  /// @param numBytes 输出参数。表示定位后的媒体流数据字节数。由于这是一个无效的定位操作，因此实际上并没有定位到任何数据，所以这个参数的值应该设置为0
  virtual void nullSeekStream(unsigned clientSessionId, void *streamToken,
                              double streamEndTime, u_int64_t &numBytes);

  /// @brief  用于设置媒体流的播放速率（Scale）。
  /// @param clientSessionId 客户端会话标识符
  /// @param streamToken  用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  /// @param scale  表示要设置的媒体流的播放速率。默认值为1.0，表示正常播放速率。
  virtual void setStreamScale(unsigned clientSessionId, void *streamToken, float scale);

  /// @brief  获取当前流传输的时间
  /// @param 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  virtual float getCurrentNPT(void *streamToken);

  /// @brief   获取流的数据源
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  virtual FramedSource *getStreamSource(void *streamToken);

  /// @brief  获取RTP发送器和RTCP实例
  /// @param streamToken 用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  /// @param rtpSink  用于接收结果
  /// @param rtcp  用于接收结果
  virtual void getRTPSinkandRTCP(void *streamToken,
                                 RTPSink const *&rtpSink, RTCPInstance const *&rtcp);

  /// @brief  删除流
  /// @param clientSessionId 需要删除的流会话id
  /// @param streamToken  用于存储表示媒体流的令牌。该参数将在函数内部被设置为媒体流的令牌，并将用于后续流传输操作<StreamState *>
  virtual void deleteStream(unsigned clientSessionId, void *&streamToken);

protected: // new virtual functions, possibly redefined by subclasses
  /// @brief 获取用于SDP描述的辅助行。子类可以通过重定义这个函数来提供特定的SDP描述。
  virtual char const *getAuxSDPLine(RTPSink *rtpSink,
                                    FramedSource *inputSource);

  // This routine is used to seek by relative (i.e., NPT) time.
  // "streamDuration", if >0.0, specifies how much data to stream, past "seekNPT".  (If <=0.0, all remaining data is streamed.)
  // "numBytes" returns the size (in bytes) of the data to be streamed, or 0 if unknown or unlimited.
  // 根据客户端的请求，将媒体流定位到指定的时间点（seekNPT），并返回实际定位后的时间点和数据量。
  virtual void seekStreamSource(FramedSource *inputSource, double &seekNPT, double streamDuration, u_int64_t &numBytes);

  // This routine is used to seek by 'absolute' time.
  // "absStart" should be a string of the form "YYYYMMDDTHHMMSSZ" or "YYYYMMDDTHHMMSS.<frac>Z".
  // "absEnd" should be either NULL (for no end time), or a string of the same form as "absStart".
  // These strings may be modified in-place, or can be reassigned to a newly-allocated value (after delete[]ing the original).
  // 这个函数支持以绝对时间（Absolute time）进行定位。
  virtual void seekStreamSource(FramedSource *inputSource, char *&absStart, char *&absEnd);

  // 设置数据源的播放速度
  virtual void setStreamSourceScale(FramedSource *inputSource, float scale);

  // 设置数据源的播放时长
  virtual void setStreamSourceDuration(FramedSource *inputSource, double streamDuration, u_int64_t &numBytes);

  // 关闭数据源
  virtual void closeStreamSource(FramedSource *inputSource);

protected: // new virtual functions, defined by all subclasses
  // "estBitrate" is the stream's estimated bitrate, in kbps
  // 创建新的流数据源。子类必须实现这个函数，用于创建特定类型的媒体数据源。
  virtual FramedSource *createNewStreamSource(unsigned clientSessionId,
                                              unsigned &estBitrate) = 0;

  // 创建新的RTP发送器。子类必须实现这个函数，用于创建特定类型的RTP发送器。
  virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
                                    FramedSource *inputSource) = 0;

protected: // new virtual functions, may be redefined by a subclass:
  // 创建组播地址
  virtual Groupsock *createGroupsock(struct in_addr const &addr, Port port);

  // 创建RTCP发送器
  virtual RTCPInstance *createRTCP(Groupsock *RTCPgs, unsigned totSessionBW, /* in kbps */
                                   unsigned char const *cname, RTPSink *sink);

public:
  // 设置是否将RTCP与RTP复用在同一个端口上。==> fMultiplexRTCPWithRTP = True;
  void multiplexRTCPWithRTP() { fMultiplexRTCPWithRTP = True; }
  // An alternative to passing the "multiplexRTCPWithRTP" parameter as True in the constructor

  /// @brief 设置处理RTCP "APP"包的回调函数
  void setRTCPAppPacketHandler(RTCPAppHandlerFunc *handler, void *clientData);
  // Sets a handler to be called if a RTCP "APP" packet arrives from any future client.
  // (Any current clients are not affected; any "APP" packets from them will continue to be
  // handled by whatever handler existed when the client sent its first RTSP "PLAY" command.)
  // (Call with (NULL, NULL) to remove an existing handler - for future clients only)

  /// @brief 发送自定义的RTCP "APP"包给客户端。
  void sendRTCPAppPacket(u_int8_t subtype, char const *name,
                         u_int8_t *appDependentData, unsigned appDependentDataSize);
  // Sends a custom RTCP "APP" packet to the most recent client (if "reuseFirstSource" was False),
  // or to all current clients (if "reuseFirstSource" was True).
  // The parameters correspond to their
  // respective fields as described in the RTP/RTCP definition (RFC 3550).
  // Note that only the low-order 5 bits of "subtype" are used, and only the first 4 bytes
  // of "name" are used.  (If "name" has fewer than 4 bytes, or is NULL,
  // then the remaining bytes are '\0'.)

private:
  // used to implement "sdpLines()"
  void setSDPLinesFromRTPSink(RTPSink *rtpSink, FramedSource *inputSource,
                              unsigned estBitrate);

protected:
  char *fSDPLines;                   // 用于存储SDP（Session Description Protocol）行的指针，初始值为NULL
  HashTable *fDestinationsHashTable; // 用于存储客户端会话ID对应的目标地址。当客户端请求连接并接收媒体流时，服务器将使用该哈希表来跟踪每个客户端的地址信息

private:
  Boolean fReuseFirstSource;           // 成员变量，指示是否重用第一个源,如果设置为True，则在客户端请求多个媒体流时，只使用第一个源进行传输。
  portNumBits fInitialPortNum;         // 初始端口号
  Boolean fMultiplexRTCPWithRTP;       // 成员变量，指示是否将RTCP与RTP复用在同一个端口上。
  void *fLastStreamToken;              // 用于存储最后一个流令牌的指针，初始值为NULL。
  char fCNAME[100];                    // for RTCP
  RTCPAppHandlerFunc *fAppHandlerTask; // 用于处理应用程序特定的任务和客户端数据
  void *fAppHandlerClientData;         // RTCPAppHandlerFunc参数
  friend class StreamState;
};

// A class that represents the state of an ongoing stream.  This is used only internally, in the implementation of
// "OnDemandServerMediaSubsession", but we expose the definition here, in case subclasses of "OnDemandServerMediaSubsession"
//  want to access it.
// 它用于存储RTP/RTCP传输的目标地址和端口信息，以及TCP相关信息（如果使用TCP传输）。
class Destinations
{
public:
  Destinations(struct in_addr const &destAddr,
               Port const &rtpDestPort,
               Port const &rtcpDestPort)
      : isTCP(False), addr(destAddr), rtpPort(rtpDestPort), rtcpPort(rtcpDestPort)
  {
  }
  Destinations(int tcpSockNum, unsigned char rtpChanId, unsigned char rtcpChanId)
      : isTCP(True), rtpPort(0) /*dummy*/, rtcpPort(0) /*dummy*/,
        tcpSocketNum(tcpSockNum), rtpChannelId(rtpChanId), rtcpChannelId(rtcpChanId)
  {
  }

public:
  Boolean isTCP;               // 用于指示目标地址的传输方式是TCP还是UDP
  struct in_addr addr;         // 表示目标IP地址
  Port rtpPort;                // RTP数据包的目标端口
  Port rtcpPort;               // RTCP数据包的目标端口
  int tcpSocketNum;            // TCP传输的套接字号
  unsigned char rtpChannelId;  // 表示RTP传输的通道ID。通常在TCP传输时使用
  unsigned char rtcpChannelId; // 表示RTCP传输的通道ID。通常在TCP传输时使用
};


// StreamState 类的主要目的是管理特定流媒体会话的状态和参数，以支持流媒体服务器在多个会话中同时处理多个客户端请求。
// 通过跟踪引用计数，可以确保在不再需要时及时释放资源，提高流媒体服务器的效率。
// 同时，通过管理RTP和RTCP传输相关信息，可以确保媒体数据正确地传送给客户端，并实现RTCP协议的状态管理。
class StreamState
{
public:
  StreamState(OnDemandServerMediaSubsession &master,
              Port const &serverRTPPort, Port const &serverRTCPPort,
              RTPSink *rtpSink, BasicUDPSink *udpSink,
              unsigned totalBW, FramedSource *mediaSource,
              Groupsock *rtpGS, Groupsock *rtcpGS);
  virtual ~StreamState();

  /// @brief 用于开始播放流 called by OnDemandServerMediaSubsession::startStream
  /// @param destinations 一个指向 Destinations 对象的指针，表示目标地址信息
  /// @param clientSessionId 一个无符号整数，表示客户端会话ID
  /// @param rtcpRRHandler 指向一个用于处理RTCP接收报告(RR)的处理函数指针
  /// @param rtcpRRHandlerClientData 上面那个函数指针的参数
  /// @param serverRequestAlternativeByteHandler 指向处理服务器请求替代字节（Alternative Byte）的处理程序函数的指针。这个处理程序函数会在服务器接收到客户端的替代字节请求时被调用，以便处理该请求。
  /// @param serverRequestAlternativeByteHandlerClientData 一个指向替代字节处理程序的客户端数据的指针。这个指针可以用于传递额外的信息给替代字节处理程序函数
  void startPlaying(Destinations *destinations, unsigned clientSessionId,
                    TaskFunc *rtcpRRHandler, void *rtcpRRHandlerClientData,
                    ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
                    void *serverRequestAlternativeByteHandlerClientData);

  /// @brief 用于暂停流的播放，暂停流的传输 called by OnDemandServerMediaSubsession::pauseStream
  void pause();


  /// @brief 用于发送RTCP的自定义APP包。called by OnDemandServerMediaSubsession::sendRTCPAppPacket
  /// @param subtype 一个无符号8位整数，表示RTCP的自定义APP包的子类型
  /// @param name 一个指向const char的指针，表示RTCP的自定义APP包的名称。
  /// @param appDependentData 一个指向无符号8位整数的指针，表示RTCP的自定义APP包的应用程序依赖数据。
  /// @param appDependentDataSize 一个无符号整数，表示RTCP的自定义APP包的应用程序依赖数据的大小。
  void sendRTCPAppPacket(u_int8_t subtype, char const *name,
                         u_int8_t *appDependentData, unsigned appDependentDataSize);

  /// @brief 用于结束流的播放，停止传输媒体数据 called by OnDemandServerMediaSubsession::deleteStream
  void endPlaying(Destinations *destinations, unsigned clientSessionId);

  /// @brief 用于回收资源，释放流状态对象。
  void reclaim();

  /// @brief 返回当前该流正在被使用的数量，也就是引用计数器的值
  unsigned &referenceCount() { return fReferenceCount; }

  /// @brief 返回服务器的RTP端口网络字节序
  Port const &serverRTPPort() const { return fServerRTPPort; }


  /// @brief 返回服务器RTCP端口网络字节序
  Port const &serverRTCPPort() const { return fServerRTCPPort; }

  /// @brief 返回RTP发送器(RTPSink)
  RTPSink *rtpSink() const { return fRTPSink; }

  /// @brief 返回RTCP发送器(RTCPInstance)
  RTCPInstance *rtcpInstance() const { return fRTCPInstance; }

  /// @brief 返回流的总时长
  float streamDuration() const { return fStreamDuration; }

  /// @brief 返回指向媒体源（FramedSource）的指针，用于提供媒体数据。
  FramedSource *mediaSource() const { return fMediaSource; }

  /// @brief 返回当前流的起始时间（正常播放时间）
  float &startNPT() { return fStartNPT; }

private:
  OnDemandServerMediaSubsession &fMaster; // 一个引用，表示所属的 OnDemandServerMediaSubsession 对象，即该流状态所服务的媒体会话。
  Boolean fAreCurrentlyPlaying;           // 一个布尔值，用于指示当前是否正在播放
  unsigned fReferenceCount;               // 一个无符号整数，表示对该流状态对象的引用计数。用于跟踪当前有多少个客户端使用该流状态。

  Port fServerRTPPort;  // RTP传输的服务器端口号
  Port fServerRTCPPort; // RTCP传输的服务器端口号

  RTPSink *fRTPSink;      // RTP媒体传输器
  BasicUDPSink *fUDPSink; // UDP媒体传输器

  float fStreamDuration;       // 一个浮点数，表示流的总时长。
  unsigned fTotalBW;           // 一个无符号整数，表示流的总带宽
  RTCPInstance *fRTCPInstance; // 一个指向RTCP实例的指针，用于管理RTCP协议的状态。

  FramedSource *fMediaSource; // 一个指向媒体源（FramedSource）的指针，用于提供媒体数据。
  float fStartNPT;            // initial 'normal play time'; reset after each seek 一个浮点数，表示当前流的起始时间（正常播放时间）

  Groupsock *fRTPgs;  // RTP传输的组播套接字
  Groupsock *fRTCPgs; // RTCP传输的组播套接字
};

#endif
