#include "CustomMediaClient.hh"
#include <GroupsockHelper.hh>
#include <RTSPCommon.hh>

#ifndef ZeroMemory
#define ZeroMemory bzero
#endif


CustomMediaClient*
CustomMediaClient::createNew(UsageEnvironment& env, const char* rtspURL, int verbosityLevel, const char* applicationName, portNumBits tunnelOverHTTPPortNum, int socketNumToServer)
{
    return new CustomMediaClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, socketNumToServer);
}

Boolean CustomMediaClient::parseRTSPURL(const char* url, char*& username, char*& password, NetAddress& address, portNumBits& portNum, const char** urlSuffix)
{
    do {
      // Parse the URL as "rtsp://[<username>[:<password>]@]<server-address-or-name>[:<port>][/<stream-name>]" (or "rtsps://...")
      char const* prefix1 = "rtsp://";
      unsigned const prefix1Length = 7;
      char const* prefix2 = "rtsps://";
      unsigned const prefix2Length = 8;

      portNumBits defaultPortNumber;
      char const* from;
      if (_strncasecmp(url, prefix1, prefix1Length) == 0) {
        defaultPortNumber = 554;
        from = &url[prefix1Length];
      } else if (_strncasecmp(url, prefix2, prefix2Length) == 0) {
        // useTLS();
        defaultPortNumber = 322;
        from = &url[prefix2Length];
      } else {
        envir().setResultMsg("URL does not begin with \"rtsp://\" or \"rtsps://\"");
        break;
      }

      unsigned const parseBufferSize = 100;
      char parseBuffer[parseBufferSize];

      // Check whether "<username>[:<password>]@" occurs next.
      // We do this by checking whether '@' appears before the end of the URL, or before the first '/'.
      username = password = NULL; // default return values
      char const* colonPasswordStart = NULL;
      char const* lastAtPtr = NULL;
      for (char const* p = from; *p != '\0' && *p != '/'; ++p) {
        if (*p == ':' && colonPasswordStart == NULL) {
            colonPasswordStart = p;
        } else if (*p == '@') {
            lastAtPtr = p;
        }
      }
      if (lastAtPtr != NULL) {
        // We found <username> (and perhaps <password>).  Copy them into newly-allocated result strings:
        if (colonPasswordStart == NULL || colonPasswordStart > lastAtPtr) colonPasswordStart = lastAtPtr;

        char const* usernameStart = from;
        unsigned usernameLen = colonPasswordStart - usernameStart;
        username = new char[usernameLen + 1] ; // allow for the trailing '\0'
        // copyUsernameOrPasswordStringFromURL(username, usernameStart, usernameLen);

        char const* passwordStart = colonPasswordStart;
        if (passwordStart < lastAtPtr) ++passwordStart; // skip over the ':'
        unsigned passwordLen = lastAtPtr - passwordStart;
        password = new char[passwordLen + 1]; // allow for the trailing '\0'
        // copyUsernameOrPasswordStringFromURL(password, passwordStart, passwordLen);

        from = lastAtPtr + 1; // skip over the '@'
      }

      // Next, parse <server-address-or-name>
      char* to = &parseBuffer[0];
      Boolean isInSquareBrackets = False; //  by default
      if (*from == '[') {
        ++from;
        isInSquareBrackets = True;
      }
      unsigned i;
      for (i = 0; i < parseBufferSize; ++i) {
        if (*from == '\0' ||
        (*from == ':' && !isInSquareBrackets) ||
        *from == '/' ||
        (*from == ']' && isInSquareBrackets)) {
          // We've completed parsing the address
          *to = '\0';
          if (*from == ']' && isInSquareBrackets) ++from;
          break;
        }
        *to++ = *from++;
      }
      if (i == parseBufferSize) {
        envir().setResultMsg("URL is too long");
        break;
      }

      NetAddressList addresses(parseBuffer);
      if (addresses.numAddresses() == 0) {
        envir().setResultMsg("Failed to find network address for \"",
                 parseBuffer, "\"");
        break;
      }
      address = *(addresses.firstAddress());

      portNum = defaultPortNumber; // unless it's specified explicitly in the URL
      char nextChar = *from;
      if (nextChar == ':') {
        int portNumInt;
        if (sscanf(++from, "%d", &portNumInt) != 1) {
          envir().setResultMsg("No port number follows ':'");
          break;
        }
        if (portNumInt < 1 || portNumInt > 65535) {
          envir().setResultMsg("Bad port number");
          break;
        }
        portNum = (portNumBits)portNumInt;
        while (*from >= '0' && *from <= '9') ++from; // skip over port number
      }

      // The remainder of the URL is the suffix:
      if (urlSuffix != NULL) *urlSuffix = from;

      return True;
    } while (0);

    // An error occurred in the parsing:
    return False;
}

CustomMediaClient::CustomMediaClient(UsageEnvironment &env, const char* rtspURL, int verbosityLevel, const char* applicationName, portNumBits tunnelOverHTTPPortNum, int socketNumToServer)
  : Medium(env), fRTPInterface(NULL), fRTPgs(NULL), fBaseURL(NULL), fFrameIndex(-1), fFrameBuffer(NULL), fFrameHead(0), fFrameTail(0),
    _frameQueue(new std::deque<_custom_frame>()), fPacketReadInProgress(NULL), fAreDoingNetworkReads(False)
{
    fBaseURL = strDup(rtspURL);
}

CustomMediaClient::~CustomMediaClient()
{
    if (fRTPInterface) {
        fRTPInterface->stopNetworkReading();
        delete fRTPInterface;
    }

    if (fRTPgs) {
        delete fRTPgs;
    }

    if (fBaseURL) {
        delete[] fBaseURL;
    }

    if (_frameQueue) {
      for (_custom_frame& frame: (*_frameQueue)) {
        delete[] frame.data;
      }
      _frameQueue->clear();
      delete _frameQueue;
    }

    if (fFrameBuffer) {
      delete[] fFrameBuffer;
    }

    if (fPacketReadInProgress) {
        delete fPacketReadInProgress;
    }
}

void CustomMediaClient::enqueueFrame(_custom_frame frame)
{
  if (frame.data && _frameQueue->size() < 300) {
    _frameQueue->push_back(frame);
  } else if (frame.data) {
    // Drop frame
    delete[] frame.data;
  }
}

_custom_frame CustomMediaClient::dequeueFrame()
{
  if (_frameQueue->size()) {
    _custom_frame frame = _frameQueue->front();
    _frameQueue->pop_front();
    return frame;
  }
  return _custom_frame { nullptr, 0 };
}

Boolean CustomMediaClient::connectToServer()
{
    char* username;
    char* password;

    NetAddress  destAddress;
    portNumBits urlPortNum;

    if (!parseRTSPURL(fBaseURL, username, password, destAddress, urlPortNum, NULL)) return False;

    delete[] username;
    delete[] password;

    const Port rtpPort(urlPortNum);
    const unsigned char ttl = 255;

    struct sockaddr_storage servaddr;
    ZeroMemory(&servaddr, sizeof(servaddr));

    copyAddress(servaddr, &destAddress);
    setPortNum(servaddr, htons(urlPortNum));

    fRTPgs = new Groupsock(envir(), servaddr, rtpPort, ttl);
    // fRTPgs->multicastSendOnly();

    fRTPInterface = new RTPInterface(this, fRTPgs);

    // Try to use a big receive buffer for RTP:
    increaseReceiveBufferTo(envir(), fRTPgs->socketNum(), 50*1024);

    if (!fAreDoingNetworkReads) {
      // Turn on background read handling of incoming packets:
      fAreDoingNetworkReads = True;

      TaskScheduler::BackgroundHandlerProc* handler
        = (TaskScheduler::BackgroundHandlerProc*)&networkReadHandler;

      fRTPInterface->startNetworkReading(handler);
    }

    // char const* dummy = "Live 555\n";
    // fRTPInterface->sendPacket((unsigned char*)dummy, strlen(dummy));

    return True;
}

void CustomMediaClient::connectionHandler(void* instance, int /*mask*/)
{
    CustomMediaClient* client = (CustomMediaClient*)instance;
    // client->asyncConnectionHandler();
}

void CustomMediaClient::networkReadHandler(void* instance, int /*mask*/) {
    CustomMediaClient* client = (CustomMediaClient*)instance;
    client->asyncNetworkReadHandler();
}

void CustomMediaClient::asyncNetworkReadHandler()
{
    BufferedPacket* bPacket = fPacketReadInProgress;
    if (bPacket == NULL) {
      // Normal case: Get a free BufferedPacket descriptor to hold the new network packet:
      bPacket = new BufferedPacket();
    }

    // Read the network packet, and perform sanity checks on the RTP header:
    Boolean readSuccess = False;
    do {
      struct sockaddr_storage fromAddress;
      Boolean packetReadWasIncomplete = fPacketReadInProgress != NULL;
      if (!bPacket->fillInData(*fRTPInterface, fromAddress, packetReadWasIncomplete)) {
        if (bPacket->bytesAvailable() == 0) { // should not happen??
            envir() << "MultiFramedRTPSource internal error: Hit limit when reading incoming packet over TCP\n";
        }
        fPacketReadInProgress = NULL;
        break;
      }
      if (packetReadWasIncomplete) {
        // We need additional read(s) before we can process the incoming packet:
        fPacketReadInProgress = bPacket;
        return;
      } else {
        fPacketReadInProgress = NULL;
      }

      readSuccess = True;
    } while (0);

    // process bPacket
    processNetworkPacket(bPacket);

    if (bPacket) delete bPacket;

    // doGetNextFrame1();
    // If we didn't get proper data this time, we'll get another chance
}

u_int8_t readByte(BufferedPacket* packet) {
  u_int8_t data = *(u_int8_t*)(packet->data());
  packet->skip(1);
  return data;
}

u_int16_t readWord(BufferedPacket* packet) {
  u_int16_t data = *(u_int16_t*)(packet->data());
  packet->skip(2);
  return data;
}

u_int32_t readInt(BufferedPacket* packet) {
  u_int32_t data = *(u_int32_t*)(packet->data());
  packet->skip(4);
  return data;
}

u_int8_t* readData(BufferedPacket* packet, size_t n) {
  u_int8_t* data = new u_int8_t[n];
  memcpy(data, packet->data(), n);
  return data;
}

void CustomMediaClient::processNetworkPacket(BufferedPacket* packet)
{
    if (!packet || packet->dataSize() < 12) return;

    // ntohl(*(u_int32_t*)(packet->data())); 

    int _meta = readWord(packet);

    int _frameI = (int)readInt(packet);
    int _length = (int)readInt(packet);
    int _offset = (int)readInt(packet);

    if (fFrameIndex != _frameI) {
      if (fFrameBuffer) {
        // Full frame ??
        if (fFrameHead == fFrameTail) {
          enqueueFrame({ fFrameBuffer, fFrameTail });
        } else {
          // Just drop frame
          delete[] fFrameBuffer;
        }
      }

      fFrameIndex  = _frameI;
      fFrameBuffer = new char[_length];

      fFrameHead = 0;
      fFrameTail = _length;
    }

    if (_offset >= fFrameHead && fFrameHead != fFrameTail) {
      memcpy(&fFrameBuffer[fFrameHead], packet->data(), packet->dataSize());
      fFrameHead += packet->dataSize();

      if (fFrameHead == fFrameTail) {
        enqueueFrame({ fFrameBuffer, fFrameTail });
        fFrameBuffer = nullptr;
      }

      //envir() << "Recvd packet [ " << _frameI << "] = " 
      //        << packet->dataSize() << "\n"
      //        << "  Offset = " << _offset << "\n"
      //        << "  Length = " << _length << "\n";
    }

    // fflush(stderr);
}
