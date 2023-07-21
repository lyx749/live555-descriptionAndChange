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
// A data structure that represents a session that consists of
// potentially multiple (audio and/or video) sub-sessions
// (This data structure is used for media *streamers* - i.e., servers.
//  For media receivers, use "MediaSession" instead.)
// C++ header

#ifndef _SERVER_MEDIA_SESSION_HH
#define _SERVER_MEDIA_SESSION_HH

#ifndef _RTCP_HH
#include "RTCP.hh"
#endif

class ServerMediaSubsession; // forward

/// @brief 相当于同类型的ServerMediaSubsession的容器集合，用于执行底层流媒体传输和状态维护
class ServerMediaSession : public Medium
{
public:
  /// @brief 静态方法创造一个ServerMediaSession并返回
  /// @param env 用户基础环境变量
  /// @param streamName <会话流名字>资源路径，也是在 GenericMediaServer 中，通过 HashTable 来维护时，所用的 key
  /// @param info 资源路径
  /// @param description 含有流媒体类型的一个字符串
  /// @param isSSM 是否为特定源多播
  /// @param miscSDPLines 感觉为一个其他的sdplines
  /// @return 返回创建的ServerMediaSession的指针
  static ServerMediaSession *createNew(UsageEnvironment &env,
                                       char const *streamName = NULL,
                                       char const *info = NULL,
                                       char const *description = NULL,
                                       Boolean isSSM = False,
                                       char const *miscSDPLines = NULL);

  /// @brief 通过MediumName从MediumTable中查找对应ServerMediaSession
  /// @param env 用户基础环境变量
  /// @param mediumName 需要查找的名字
  /// @param resultSession 查找结果返回
  /// @return success true，failed false
  static Boolean lookupByName(UsageEnvironment &env,
                              char const *mediumName,
                              ServerMediaSession *&resultSession);

  /// @brief 遍历该ServerMediaSession中的所有ServerMediaSubSession生成sdp信息
  /// @return 返回一个字符串，里面包含sdp信息
  char *generateSDPDescription(); // based on the entire session
                                  // Note: The caller is responsible for freeing the returned string

  /// @brief 返回会话流名字，同样也是资源路径
  /// @return 通过char const*返回
  char const *streamName() const { return fStreamName; }

  /// @brief 添加子session
  /// @param subsession 需要添加的子session
  /// @return success true，failed false
  Boolean addSubsession(ServerMediaSubsession *subsession);

  /// @brief 返回子session的数量
  /// @return 返回数量
  unsigned numSubsessions() const { return fSubsessionCounter; }

  /// @brief sets "scale" to the actual supported scale
  void testScaleFactor(float &scale);

  // a result == 0 means an unbounded session (the default)
  // a result < 0 means: subsession durations differ; the result is -(the largest).
  // a result > 0 means: this is the duration of a bounded session
  float duration() const;

  // called whenever a client - accessing this media - notes liveness.
  // The default implementation does nothing, but subclasses can redefine this - e.g., if you
  // want to remove long-unused "ServerMediaSession"s from the server.
  virtual void noteLiveness();

  /// @brief 返回当前引用次数
  /// @return 引用次数
  unsigned referenceCount() const { return fReferenceCount; }

  /// @brief 将引用计数器加一
  void incrementReferenceCount() { ++fReferenceCount; }

  /// @brief 将引用计数器减一
  void decrementReferenceCount()
  {
    if (fReferenceCount > 0)
      --fReferenceCount;
  }
  /// @brief 如果当前引用计数器为0返回true，否则为false，代表是否要删除
  Boolean &deleteWhenUnreferenced() { return fDeleteWhenUnreferenced; }

  // Removes and deletes all subsessions added by "addSubsession()", returning us to an 'empty' state
  // Note: If you have already added this "ServerMediaSession" to a "RTSPServer" then, before calling this function,
  //   you must first close any client connections that use it,
  //   by calling "RTSPServer::closeAllClientSessionsForServerMediaSession()".
  void deleteAllSubsessions();

protected:
  // called only by "createNew()"
  ServerMediaSession(UsageEnvironment &env, char const *streamName,
                     char const *info, char const *description,
                     Boolean isSSM, char const *miscSDPLines);

  virtual ~ServerMediaSession();

private: // redefined virtual functions
  virtual Boolean isServerMediaSession() const;

private:
  Boolean fIsSSM; // 是否为特定源多播

  // Linkage fields:
  friend class ServerMediaSubsessionIterator;
  ServerMediaSubsession *fSubsessionsHead; // 子会话单链表表头
  ServerMediaSubsession *fSubsessionsTail; // 子会话单链表表尾
  unsigned fSubsessionCounter;             // 子会话个数

  char *fStreamName;               // 当前流名字，也代表资源路径
  char *fInfoSDPString;            // 会话的信息SDP字符串，通常是描述性的字符串。
  char *fDescriptionSDPString;     // 会话的描述SDP字符串，通常是详细的描述性字符串
  char *fMiscSDPLines;             // 其他的SDP行，用于自定义SDP字符串中的其他信息。
  struct timeval fCreationTime;    // 会话的创建时间，记录会话创建的时间戳。
  unsigned fReferenceCount;        // 引用计数器，用于跟踪会话的引用数。
  Boolean fDeleteWhenUnreferenced; // 一个布尔值，指示当没有引用时是否删除会话
};

/// @brief 访问ServerMediaSession下的子会话的迭代器
class ServerMediaSubsessionIterator
{
public:
  ServerMediaSubsessionIterator(ServerMediaSession &session);
  virtual ~ServerMediaSubsessionIterator();

  ServerMediaSubsession *next(); // NULL if none
  void reset();

private:
  ServerMediaSession &fOurSession;
  ServerMediaSubsession *fNextPtr;
};

/// @brief 具体源的子会话
class ServerMediaSubsession : public Medium
{
public:

  //子会话的跟踪号码，用于生成payload
  unsigned trackNumber() const { return fTrackNumber; }
  char const *trackId();

  /// @brief 生成该源的sdp，纯虚函数具体实现在子类OnDemandServerMediaSubsesion
  virtual char const *sdpLines() = 0;
  /// @brief 获取流参数，纯虚函数，具体实现在子类OnDemandServerMediaSubsesion
  virtual void getStreamParameters(unsigned clientSessionId,           // in
                                   netAddressBits clientAddress,       // in
                                   Port const &clientRTPPort,          // in
                                   Port const &clientRTCPPort,         // in
                                   int tcpSocketNum,                   // in (-1 means use UDP, not TCP)
                                   unsigned char rtpChannelId,         // in (used if TCP)
                                   unsigned char rtcpChannelId,        // in (used if TCP)
                                   netAddressBits &destinationAddress, // in out
                                   u_int8_t &destinationTTL,           // in out
                                   Boolean &isMulticast,               // out
                                   Port &serverRTPPort,                // out
                                   Port &serverRTCPPort,               // out
                                   void *&streamToken                  // out
                                   ) = 0;

  /// @brief 开始播放，具体实现在子类
  virtual void startStream(unsigned clientSessionId, void *streamToken,
                           TaskFunc *rtcpRRHandler,
                           void *rtcpRRHandlerClientData,
                           unsigned short &rtpSeqNum,
                           unsigned &rtpTimestamp,
                           ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
                           void *serverRequestAlternativeByteHandlerClientData) = 0;

  /// @brief 停止播放，具体实现在子类
  virtual void pauseStream(unsigned clientSessionId, void *streamToken);

  // This routine is used to seek by relative (i.e., NPT) time.
  // "streamDuration", if >0.0, specifies how much data to stream, past "seekNPT".  (If <=0.0, all remaining data is streamed.)
  // "numBytes" returns the size (in bytes) of the data to be streamed, or 0 if unknown or unlimited.
  virtual void seekStream(unsigned clientSessionId, void *streamToken, double &seekNPT,
                          double streamDuration, u_int64_t &numBytes);

  // This routine is used to seek by 'absolute' time.
  // "absStart" should be a string of the form "YYYYMMDDTHHMMSSZ" or "YYYYMMDDTHHMMSS.<frac>Z".
  // "absEnd" should be either NULL (for no end time), or a string of the same form as "absStart".
  // These strings may be modified in-place, or can be reassigned to a newly-allocated value (after delete[]ing the original).
  virtual void seekStream(unsigned clientSessionId, void *streamToken, char *&absStart, char *&absEnd);

  // Called whenever we're handling a "PLAY" command without a specified start time.
  virtual void nullSeekStream(unsigned clientSessionId, void *streamToken,
                              double streamEndTime, u_int64_t &numBytes);

  virtual void setStreamScale(unsigned clientSessionId, void *streamToken, float scale);

  /// @brief （Normalized Play Time）是一种在流媒体中表示规范化播放时间的时间格式。NPT是一种相对时间表示方法，用于标识媒体流的播放位置或时间范围。
  /// @param streamToken 流的标识
  /// @return NPT
  virtual float getCurrentNPT(void *streamToken);

  virtual FramedSource *getStreamSource(void *streamToken);

  // Returns pointers to the "RTPSink" and "RTCPInstance" objects for "streamToken".
  // (This can be useful if you want to get the associated 'Groupsock' objects, for example.)
  // You must not delete these objects, or start/stop playing them; instead, that is done
  // using the "startStream()" and "deleteStream()" functions.
  virtual void getRTPSinkandRTCP(void *streamToken,
                                 RTPSink const *&rtpSink, RTCPInstance const *&rtcp) = 0;

  virtual void deleteStream(unsigned clientSessionId, void *&streamToken);
  // sets "scale" to the actual supported scale
  virtual void testScaleFactor(float &scale);

  // returns 0 for an unbounded session (the default)
  // returns > 0 for a bounded session
  virtual float duration() const;

  // Subclasses can reimplement this iff they support seeking by 'absolute' time.
  virtual void getAbsoluteTimeRange(char *&absStartTime, char *&absEndTime) const;

  // The following may be called by (e.g.) SIP servers, for which the
  // address and port number fields in SDP descriptions need to be non-zero:
  void setServerAddressAndPortForSDP(netAddressBits addressBits,
                                     portNumBits portBits);

protected: // we're a virtual base class
  ServerMediaSubsession(UsageEnvironment &env);
  virtual ~ServerMediaSubsession();

  char const *rangeSDPLine() const;
  // returns a string to be delete[]d

  ServerMediaSession *fParentSession;  // 父会话
  netAddressBits fServerAddressForSDP; // 服务器地址(网络字节序)
  portNumBits fPortNumForSDP;          // 服务器端口号(网络字节序)

private:
  friend class ServerMediaSession;
  friend class ServerMediaSubsessionIterator;
  ServerMediaSubsession *fNext;

  unsigned fTrackNumber; // within an enclosing ServerMediaSession
  char const *fTrackId;
};

#endif
