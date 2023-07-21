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
// A generic media server class, used to implement a RTSP server, and any other server that uses
//  "ServerMediaSession" objects to describe media to be served.
// C++ header

#ifndef _GENERIC_MEDIA_SERVER_HH
#define _GENERIC_MEDIA_SERVER_HH

#ifndef _MEDIA_HH
#include "Media.hh"
#endif
#ifndef _SERVER_MEDIA_SESSION_HH
#include "ServerMediaSession.hh"
#endif

#ifndef REQUEST_BUFFER_SIZE
#define REQUEST_BUFFER_SIZE 20000 // request最大的buffersize
#endif
#ifndef RESPONSE_BUFFER_SIZE
#define RESPONSE_BUFFER_SIZE 20000 // response最大的buffsize
#endif

class GenericMediaServer : public Medium
{
public:
  /// @brief 添加具体的session到fServerMediaSession(hashTable)
  /// @param serverMediaSession 该类为当前文件格式的一个serverMediaSession，该serverMediaSession下有对应该文件格式的子session
  void addServerMediaSession(ServerMediaSession *serverMediaSession);

  /// @brief 通过streamName查找对应的serverMediaSession(虚函数只是做了简单实现)
  /// @param streamName 需要查找的serverMediaSession名字
  /// @param isFirstLookupInSession 是否为第一次查找该serverMediaSession
  /// @return ServerMediaSession* 查找到的serverMediaSession
  virtual ServerMediaSession *
  lookupServerMediaSession(char const *streamName, Boolean isFirstLookupInSession = True);

  /// @brief 通过需要移除的serverMediaSession的地址移除对应的serverMediaSession
  /// @param serverMediaSession 需要移除的serverMediaSession
  void removeServerMediaSession(ServerMediaSession *serverMediaSession);
  // Removes the "ServerMediaSession" object from our lookup table, so it will no longer be accessible by new clients.
  // (However, any *existing* client sessions that use this "ServerMediaSession" object will continue streaming.
  //  The "ServerMediaSession" object will not get deleted until all of these client sessions have closed.)
  // (To both delete the "ServerMediaSession" object *and* close all client sessions that use it,
  //  call "deleteServerMediaSession(serverMediaSession)" instead.)

  /// @brief 根据需要移除的serverSession的名字来移除对应的serverSession
  /// @param streamName 需要移除的ServerSession的名字
  virtual void removeServerMediaSession(char const *streamName);
  // ditto

  /// @brief 关闭所有正在使用serverMediaSession的ClientSession
  /// @param serverMediaSession 对应的serverMediaSession
  void closeAllClientSessionsForServerMediaSession(ServerMediaSession *serverMediaSession);
  // Closes (from the server) all client sessions that are currently using this "ServerMediaSession" object.
  // Note, however, that the "ServerMediaSession" object remains accessible by new clients.

  /// @brief 关闭所有正在使用名字为streamName的serverMediaSession的ClientSession
  /// @param streamName 对应的serverMediaSession的名字
  virtual void closeAllClientSessionsForServerMediaSession(char const *streamName);
  // ditto

  /// @brief 关闭所有正在使用serverMediaSession的ClientSession，并且移除serverMediaSession(相当于先调用closeAllClientSessionsForServerMediaSession再调用removeServerMediaSession)
  /// @param serverMediaSession
  void deleteServerMediaSession(ServerMediaSession *serverMediaSession);
  // Equivalent to:
  //     "closeAllClientSessionsForServerMediaSession(serverMediaSession); removeServerMediaSession(serverMediaSession);"
  virtual void deleteServerMediaSession(char const *streamName);
  // Equivalent to:
  //     "closeAllClientSessionsForServerMediaSession(streamName); removeServerMediaSession(streamName);

  /// @brief 返回ClientSession的个数
  /// @return ClientSession的个数
  unsigned numClientSessions() const { return fClientSessions->numEntries(); }

protected:
  // If "reclamationSeconds" > 0, then the "ClientSession" state for each client will get
  // reclaimed if no activity from the client is detected in at least "reclamationSeconds".
  // we're an abstract base class
  GenericMediaServer(UsageEnvironment &env, int ourSocket, Port ourPort,
                     unsigned reclamationSeconds);

  virtual ~GenericMediaServer();
  
  
  /// @brief 关闭所有的ServerMediaSession，ClientSession，ClientConnection
  void cleanup(); // MUST be called in the destructor of any subclass of us


  /// @brief 根据传入的端口号生成对应的监听套接字(假如传入的port为0，内核将会为我们分配对应的端口，并且将端口号赋给fServerPort成员变量)
  /// @param env 用户基础环境变量
  /// @param ourPort 传入的端口号
  /// @return 生产的监听套接字
  static int setUpOurSocket(UsageEnvironment &env, Port &ourPort);

  /// @brief 处理到来的连接的回调函数(一个封装，本质是调用incomingConnectionHandler)
  /// @param  函数参数，占位参数
  /// @param  函数参数，占位参数
  static void incomingConnectionHandler(void *, int /*mask*/);


  /// @brief 处理到来的连接由incomingConnectionHandler静态封装函数调用
  void incomingConnectionHandler();


  /// @brief 理到来的连接由incomingConnectionHandler函数调用，创建对应的clientConnection来保存TCP连接信息
  /// @param serverSocket Server对应的监听套接字
  void incomingConnectionHandlerOnSocket(int serverSocket);

public: // should be protected, but some old compilers complain otherwise
  // The state of a TCP connection used by a client:

  /// @brief 保存TCP连接信息，每一个连接都有一个，用于处理网络 I/O，处理 RTSP 请求，并建立会话
  class ClientConnection
  {
  protected:
    ClientConnection(GenericMediaServer &ourServer, int clientSocket, struct sockaddr_in clientAddr);
    virtual ~ClientConnection();

    UsageEnvironment &envir() { return fOurServer.envir(); }
    void closeSockets();

    
    /// @brief  处理对应连接的读数据函数，一个静态封装，调用的是成员函数incomingRequestHandler()
    /// @param  函数参数
    /// @param  函数参数
    static void incomingRequestHandler(void *, int /*mask*/);


    /// @brief 处理对应连接的读数据函数，被封装的静态函数incomingRequestHandler()调用
    void incomingRequestHandler();


    /// @brief 处理接受到的数据纯虚函数，具体实现在RTPServer下的RTSPServerClientConnection
    /// @param newBytesRead 读取的数据字节数
    virtual void handleRequestBytes(int newBytesRead) = 0;


    /// @brief 将fRequestBytesAlreadySeen(已经接收到的数据)归0，将fRequestBufferBytesLeft(接收buffer还剩多少空间)设成buffer的大小
    void resetRequestBuffer();

  protected:
    friend class GenericMediaServer;
    friend class ClientSession;
    friend class RTSPServer; // needed to make some broken Windows compilers work; remove this in the future when we end support for Windows
    GenericMediaServer &fOurServer; //保存GenericMediaServer
    int fOurSocket;                 //该连接的sockfd
    struct sockaddr_in fClientAddr; //该连接的客户端地址
    unsigned char fRequestBuffer[REQUEST_BUFFER_SIZE];  //接收buffer，大小为REQUEST_BUFFER_SIZE(20000)
    unsigned char fResponseBuffer[RESPONSE_BUFFER_SIZE];//发送buffer，大小为RESPONSE_BUFFER_SIZE(20000)

    //fRequestBytesAlreadySeen(接收数据在缓冲区的起始位置)，fRequestBufferBytesLeft(接收buffer还剩多少空间)
    unsigned fRequestBytesAlreadySeen, fRequestBufferBytesLeft;
  };

  // The state of an individual client session (using one or more sequential TCP connections) handled by a server:
  /// @brief 用于封装整个流媒体会话，处理那些要求流媒体会话已经建立的 RTSP 请求，如 PLAY 等
  class ClientSession
  {
  protected:
    ClientSession(GenericMediaServer &ourServer, u_int32_t sessionId);
    virtual ~ClientSession();

    UsageEnvironment &envir() { return fOurServer.envir(); }
    void noteLiveness();
    static void noteClientLiveness(ClientSession *clientSession);
    static void livenessTimeoutTask(ClientSession *clientSession);

  protected:
    friend class GenericMediaServer;
    friend class ClientConnection;
    GenericMediaServer &fOurServer; //保存GenericMediaServer
    u_int32_t fOurSessionId;        //该会话id
    ServerMediaSession *fOurServerMediaSession; //该会话的ServerMediaSession
    TaskToken fLivenessCheckTask;
  };

protected:
  /// @brief 纯虚函数，通过clientSocket和clientAddr创建对应的ClientConnection，具体实现在RTSPServer类中
  /// @param clientSocket 客户套接字描述符
  /// @param clientAddr 客户端地址
  /// @return 返回ClientConnection *
  virtual ClientConnection *createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr) = 0;


  /// @brief 纯虚函数，通过sessionId创建对应的ClientConnection，具体实现在RTSPServer类中
  /// @param sessionId 需要创建的会话id
  /// @return 返回ClientSession *
  virtual ClientSession *createNewClientSession(u_int32_t sessionId) = 0;


  /// @brief 随机生产一个seesionId，然后通过该id创建对应的ClientSession
  /// @return 返回ClientSession *
  ClientSession *createNewClientSessionWithId();
  // Generates a new (unused) random session id, and calls the "createNewClientSession()"
  // virtual function with this session id as parameter.

  // Lookup a "ClientSession" object by sessionId (integer, and string):
  ClientSession *lookupClientSession(u_int32_t sessionId);
  ClientSession *lookupClientSession(char const *sessionIdStr);

  // An iterator over our "ServerMediaSession" objects:
  class ServerMediaSessionIterator
  {
  public:
    ServerMediaSessionIterator(GenericMediaServer &server);
    virtual ~ServerMediaSessionIterator();
    ServerMediaSession *next();

  private:
    HashTable::Iterator *fOurIterator;
  };

protected:
  friend class ClientConnection;
  friend class ClientSession;
  friend class ServerMediaSessionIterator;
  int fServerSocket;    //server的监听套接字
  Port fServerPort;     //server监听端口


  /**
   * 该参数类似于心跳包机制，在fReclamationSeconds时间内，没有收到客户端的RTSP命令或者RTCP的“RR"数据包，则将会回收该客户端的RTSPClientSeesion
  */
  unsigned fReclamationSeconds; //回收秒数

private:
  HashTable *fServerMediaSessions; // ServerMediaSession的哈希表，以ServerMediaSession的名字为key，以ServerMediaSession的地址为值
  HashTable *fClientConnections;   // the "ClientConnection" objects that we're using
  HashTable *fClientSessions;      // maps 'session id' strings to "ClientSession" objects
  u_int32_t fPreviousClientSessionId; //上一个客户会话id
};

// A data structure used for optional user/password authentication:

class UserAuthenticationDatabase
{
public:
  UserAuthenticationDatabase(char const *realm = NULL,
                             Boolean passwordsAreMD5 = False);
  // If "passwordsAreMD5" is True, then each password stored into, or removed from,
  // the database is actually the value computed
  // by md5(<username>:<realm>:<actual-password>)
  virtual ~UserAuthenticationDatabase();

  virtual void addUserRecord(char const *username, char const *password);
  virtual void removeUserRecord(char const *username);

  virtual char const *lookupPassword(char const *username);
  // returns NULL if the user name was not present

  char const *realm() { return fRealm; }
  Boolean passwordsAreMD5() { return fPasswordsAreMD5; }

protected:
  HashTable *fTable;
  char *fRealm;
  Boolean fPasswordsAreMD5;
};

#endif
