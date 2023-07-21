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
// An abstraction of a network interface used for RTP (or RTCP).
// (This allows the RTP-over-TCP hack (RFC 2326, section 10.12) to
// be implemented transparently.)
// C++ header

#ifndef _RTP_INTERFACE_HH
#define _RTP_INTERFACE_HH

#ifndef _MEDIA_HH
#include <Media.hh>
#endif
#ifndef _GROUPSOCK_HH
#include "Groupsock.hh"
#endif

// Typedef for an optional auxilliary handler function, to be called
// when each new packet is read:
typedef void AuxHandlerFunc(void *clientData, unsigned char *packet,
                            unsigned &packetSize);

// A hack that allows a handler for RTP/RTCP packets received over TCP to process RTSP commands that may also appear within
// the same TCP connection.  A RTSP server implementation would supply a function like this - as a parameter to
// "ServerMediaSubsession::startStream()".
typedef void ServerRequestAlternativeByteHandler(void *instance, u_int8_t requestByte);

/// @brief 表示TCP流记录,通过使用tcpStreamRecord类，可以创建和管理TCP流记录的链表结构
class tcpStreamRecord
{
public:
  tcpStreamRecord(int streamSocketNum, unsigned char streamChannelId,
                  tcpStreamRecord *next);
  virtual ~tcpStreamRecord();

public:
  tcpStreamRecord *fNext;
  /**
   * 通常情况下，一个 TCP 连接中可以有多个逻辑通道（通常称为通道或流），通过不同的通道进行并行传输。这些通道可以用于多路复用数据，
   * 使得在同一个 TCP 连接上可以同时传输多个独立的数据流。每个通道都有一个唯一的通道 ID，以区分不同的数据流。\
   */
  int fStreamSocketNum;           // 表示TCP流的套接字号
  unsigned char fStreamChannelId; // 表示TCP流的通道ID
};

/// @brief 用于处理RTP数据包的发送和接收。它支持使用UDP和TCP协议进行传输，并提供了相应的回调函数和接口，以便处理接收到的数据。通过使用该类，可以实现对RTP数据的灵活控制和处理。
class RTPInterface
{
public:
  RTPInterface(Medium *owner, Groupsock *gs);
  virtual ~RTPInterface();

  /// @brief 返回与该RTP接口关联的Groupsock对象
  Groupsock *gs() const { return fGS; }

  /// @brief 设置流套接字和流通道ID
  /// @param sockNum 流套接字
  /// @param streamChannelId 流通道ID
  void setStreamSocket(int sockNum, unsigned char streamChannelId);

  /// @brief 添加流套接字和流通道ID
  /// @param sockNum 流套接字
  /// @param streamChannelId 流通道ID
  void addStreamSocket(int sockNum, unsigned char streamChannelId);

  /// @brief 删除流套接字下的流通到id，如果该套接字下所有的流通道id都被删除了，则删除这个套接字
  /// @param sockNum 流套接字
  /// @param streamChannelId 流通道ID
  void removeStreamSocket(int sockNum, unsigned char streamChannelId);

  /// @brief 根据套接字 socketNum从SocketTable中查找到对应的SocketDescriptor，并且设置  fServerRequestAlternativeByteHandler = handler;
  ///        fServerRequestAlternativeByteHandlerClientData = clientData;
  static void setServerRequestAlternativeByteHandler(UsageEnvironment &env, int socketNum,
                                                     ServerRequestAlternativeByteHandler *handler, void *clientData);

  /// @brief  根据套接字 socketNum从SocketTable中查找到对应的SocketDescriptor，并且清除fServerRequestAlternativeByteHandler和fServerRequestAlternativeByteHandlerClientData
  static void clearServerRequestAlternativeByteHandler(UsageEnvironment &env, int socketNum);

  /// @brief 发送数据包给对应的组播成员以及发送数据包给tcp连接
  /// @param packet 需要发送的数据包
  /// @param packetSize 需要发送数据包的大小
  /// @return success true，failed false
  Boolean sendPacket(unsigned char *packet, unsigned packetSize);

  /// @brief 将tcp连接，和udp组播成员均加入到select中，并设置读事件监听
  /// @param handlerProc udp读数据的回调函数
  void startNetworkReading(TaskScheduler::BackgroundHandlerProc *
                               handlerProc);

  /// @brief 处理读取数据。根据读取的来源（UDP或TCP），返回相应的信息。
  /// @param buffer 读数据缓冲区
  /// @param bufferMaxSize  读数据缓冲区大小
  /// @param bytesRead  已经读取的数据大小
  /// @param fromAddress  发送数据的ip，用于接收客户端ip地址
  /// @param tcpSocketNum 如果是tcp数据流，则这个变量用于接收tcp连接的套接字
  /// @param tcpStreamChannelId 如果是tcp数据流，则这个变量用于接收tcp连接的数据逻辑通道号
  /// @param packetReadWasIncomplete  是否完全读取
  Boolean handleRead(unsigned char *buffer, unsigned bufferMaxSize,
                     // out parameters:
                     unsigned &bytesRead, struct sockaddr_in &fromAddress,
                     int &tcpSocketNum, unsigned char &tcpStreamChannelId,
                     Boolean &packetReadWasIncomplete);
  // Note: If "tcpSocketNum" < 0, then the packet was received over UDP, and "tcpStreamChannelId"
  //   is undefined (and irrelevant).

  // Otherwise (if "tcpSocketNum" >= 0), the packet was received (interleaved) over TCP, and
  //   "tcpStreamChannelId" will return the channel id.

  /// @brief 停止网络数据读取
  void stopNetworkReading();

  UsageEnvironment &envir() const { return fOwner->envir(); }

  /// @brief 设置辅助读取处理器。用于设置辅助读取数据的回调函数。
  void setAuxilliaryReadHandler(AuxHandlerFunc *handlerFunc,
                                void *handlerClientData)
  {
    fAuxReadHandlerFunc = handlerFunc;
    fAuxReadHandlerClientData = handlerClientData;
  }

  // This may be called - *only immediately prior* to deleting this - to prevent our destructor
  // from turning off background reading on the 'groupsock'.  (This is in case the 'groupsock'
  // is also being read from elsewhere.)
  void forgetOurGroupsock() { fGS = NULL; }

private:
  // Helper functions for sending a RTP or RTCP packet over a TCP connection:
  Boolean sendRTPorRTCPPacketOverTCP(unsigned char *packet, unsigned packetSize,
                                     int socketNum, unsigned char streamChannelId);
  Boolean sendDataOverTCP(int socketNum, u_int8_t const *data, unsigned dataSize, Boolean forceSendToSucceed);

private:
  friend class SocketDescriptor;
  Medium *fOwner;               // 指向媒体对象（Medium）的指针。用于跟踪该RTP接口所属的媒体对象。(RTPsink*)
  Groupsock *fGS;               // 指向Groupsock对象的指针。用于跟踪该RTP接口所关联的Groupsock对象，Groupsock提供了发送和接收网络数据的功能。
  tcpStreamRecord *fTCPStreams; // 指向TCP流记录（tcpStreamRecord）的指针。这是一个可选的成员，用于支持RTP-over-TCP流传输/接收。如果RTP数据是通过TCP流传输/接收的，则使用此指针来管理TCP流的记录。

  unsigned short fNextTCPReadSize; // 下一个TCP读取的数据大小。用于指示从TCP流中还有多少数据可以读取。
  // how much data (if any) is available to be read from the TCP stream
  int fNextTCPReadStreamSocketNum;                        // 下一个TCP读取的套接字号。用于指示从哪个TCP套接字读取数据。
  unsigned char fNextTCPReadStreamChannelId;              // 下一个TCP读取的流通道ID。用于指示从TCP流中哪个通道读取数据。
  TaskScheduler::BackgroundHandlerProc *fReadHandlerProc; // 指向后台处理程序（BackgroundHandlerProc）的指针。用于设置网络读取时的回调函数。

  AuxHandlerFunc *fAuxReadHandlerFunc; // 指向辅助读取处理器函数（AuxHandlerFunc）的指针。用于设置辅助读取数据时的回调函数
  void *fAuxReadHandlerClientData;     // 指向辅助读取处理器函数的客户数据（client data）的指针。用于传递给辅助读取处理器函数的附加数据。
};

#endif
