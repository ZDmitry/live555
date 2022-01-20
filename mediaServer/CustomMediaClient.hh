#ifndef _CUSTOM_MEDIA_CLIENT_HH
#define _CUSTOM_MEDIA_CLIENT_HH

#include <RTSPClient.hh>
#include <MultiFramedRTPSource.hh>

#include <deque>

struct _custom_frame {
  char*  data;
  size_t size;
};

class CustomMediaClient: public Medium {
public:
  static CustomMediaClient* createNew(UsageEnvironment& env, char const* rtspURL,
                   int verbosityLevel = 0,
                   char const* applicationName = NULL,
                   portNumBits tunnelOverHTTPPortNum = 0,
                   int socketNumToServer = -1);

  Boolean parseRTSPURL(char const* url,
               char*& username, char*& password, NetAddress& address, portNumBits& portNum, char const** urlSuffix = NULL);
      // Parses "url" as "rtsp://[<username>[:<password>]@]<server-address-or-name>[:<port>][/<stream-name>]"
      // (Note that the returned "username" and "password" are either NULL, or heap-allocated strings that the caller must later delete[].)

  _custom_frame dequeueFrame();

  virtual Boolean connectToServer();

protected:
  CustomMediaClient(UsageEnvironment& env, char const* rtspURL,
         int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, int socketNumToServer);
  virtual ~CustomMediaClient();

  void enqueueFrame(_custom_frame frame);

private:
  // Support for asynchronous connections to the server:
  static void connectionHandler(void*, int /*mask*/);
  // void asyncConnectionHandler();

  static void networkReadHandler(void*, int /*mask*/);
  void asyncNetworkReadHandler();

  void processNetworkPacket(BufferedPacket* packet);

private:
  RTPInterface* fRTPInterface;

  Groupsock*    fRTPgs;
  char*         fBaseURL;

  int     fFrameIndex;
  char*   fFrameBuffer;

  unsigned  fFrameHead;
  unsigned  fFrameTail;

  std::deque<_custom_frame>* _frameQueue;

  BufferedPacket* fPacketReadInProgress;
  Boolean fAreDoingNetworkReads;
};

#endif // _CUSTOM_MEDIA_CLIENT_HH
