#ifndef _CUSTOM_MEDIA_SERVER_HH
#define _CUSTOM_MEDIA_SERVER_HH

#include <GenericMediaServer.hh>


class CustomMediaServer: public GenericMediaServer {
public:
    static CustomMediaServer* createNew(UsageEnvironment& env, Port ourPort = 3004, unsigned reclamationSeconds = 65);


protected:
  CustomMediaServer(UsageEnvironment& env,
         int ourSocketIPv4, int ourSocketIPv6, Port ourPort,
         unsigned reclamationSeconds);
  virtual ~CustomMediaServer();


protected:
  virtual ClientConnection* createNewClientConnection(int clientSocket, const sockaddr_storage &clientAddr);
  virtual ClientSession*    createNewClientSession(u_int32_t sessionId);


public: // should be protected, but some old compilers complain otherwise
  // The state of a TCP connection used by a client:
  class MediaClientSession; // forward
  class MediaClientConnection: public GenericMediaServer::ClientConnection {
  protected:
    MediaClientConnection(CustomMediaServer& ourServer, int clientSocket, struct sockaddr_storage const& clientAddr);
    virtual ~MediaClientConnection();

    friend class CustomMediaServer;
    friend class MediaClientSession;

    virtual void handleRequestBytes(int newBytesRead);

  protected:
    Boolean  fIsActive;
    unsigned fRecursionCount;
    unsigned fScheduledDelayedTask;

    CustomMediaServer& _mediaServer; // same as ::fOurServer
    int  fAddressFamily;
    int& fClientInputSocket;         // aliased to ::fOurSocket
    int  fClientOutputSocket;
  };

  class MediaClientSession: public GenericMediaServer::ClientSession {
  protected:
    MediaClientSession(CustomMediaServer& ourServer, u_int32_t sessionId);
    virtual ~MediaClientSession();

    friend class CustomMediaServer;
    friend class MediaClientConnection;

  protected:
    CustomMediaServer& _mediaServer; // same as ::fOurServer
  };

};

#endif // _CUSTOM_MEDIA_SERVER_HH
