#include "CustomMediaServer.hh"


CustomMediaServer *CustomMediaServer::createNew(UsageEnvironment &env, Port ourPort, unsigned reclamationSeconds)
{
    int ourSocketIPv4 = setUpOurSocket(env, ourPort, AF_INET);
    int ourSocketIPv6 = setUpOurSocket(env, ourPort, AF_INET6);

    if (ourSocketIPv4 < 0 && ourSocketIPv6 < 0) return NULL;
    return new CustomMediaServer(env, ourSocketIPv4, ourSocketIPv6, ourPort, reclamationSeconds);
}

CustomMediaServer::CustomMediaServer(UsageEnvironment &env, int ourSocketIPv4, int ourSocketIPv6, Port ourPort, unsigned reclamationSeconds)
    : GenericMediaServer(env, ourSocketIPv4, ourSocketIPv6, ourPort, reclamationSeconds)
{

}

CustomMediaServer::~CustomMediaServer()
{

}

GenericMediaServer::ClientConnection*
CustomMediaServer::createNewClientConnection(int clientSocket, const sockaddr_storage &clientAddr)
{
    return new MediaClientConnection(*this, clientSocket, clientAddr);
}

GenericMediaServer::ClientSession*
CustomMediaServer::createNewClientSession(u_int32_t sessionId)
{
    return new MediaClientSession(*this, sessionId);
}

CustomMediaServer::MediaClientConnection::MediaClientConnection(CustomMediaServer& ourServer, int clientSocket, const sockaddr_storage& clientAddr)
    : GenericMediaServer::ClientConnection(ourServer, clientSocket, clientAddr), _mediaServer(ourServer),
      fClientInputSocket(fOurSocket), fClientOutputSocket(fOurSocket), fAddressFamily(clientAddr.ss_family),
      fIsActive(True), fRecursionCount(0), fScheduledDelayedTask(0)
{

}

CustomMediaServer::MediaClientConnection::~MediaClientConnection()
{

}

void CustomMediaServer::MediaClientConnection::handleRequestBytes(int newBytesRead)
{
    int numBytesRemaining = 0;
    ++fRecursionCount;

    do {
        CustomMediaServer::MediaClientSession* clientSession = NULL;

        if (newBytesRead < 0 || (unsigned)newBytesRead >= fRequestBufferBytesLeft) {
          // Either the client socket has died, or the request was too big for us.
          // Terminate this connection:
    #ifdef DEBUG
          fprintf(stderr, "MediaClientConnection[%p]::handleRequestBytes() read %d new bytes (of %d); terminating connection!\n", this, newBytesRead, fRequestBufferBytesLeft);
    #endif
          fIsActive = False;
          break;
        }

        unsigned char* req = &fRequestBuffer[fRequestBytesAlreadySeen];
        Boolean  endOfMsg = False;

    #ifdef DEBUG
        req[newBytesRead] = '\0';
        fprintf(stderr, "MediaClientConnection[%p]::handleRequestBytes() %s %d new bytes:%s\n",
            this, numBytesRemaining > 0 ? "processing" : "read", newBytesRead, ptr);
    #endif

        fRequestBufferBytesLeft  -= newBytesRead;
        fRequestBytesAlreadySeen += newBytesRead;

        if (!endOfMsg) break; // subsequent reads will be needed to complete the request

        unsigned requestSize = 0;

        numBytesRemaining = fRequestBytesAlreadySeen - requestSize;
        resetRequestBuffer(); // to prepare for any subsequent request

        if (numBytesRemaining > 0) {
          memmove(fRequestBuffer, &fRequestBuffer[requestSize], numBytesRemaining);
          newBytesRead = numBytesRemaining;
        }
    } while (numBytesRemaining > 0);

    --fRecursionCount;

    // If it has a scheduledDelayedTask, don't delete the instance or close the sockets. The sockets can be reused in the task.
    if (!fIsActive && fScheduledDelayedTask <= 0) {
      if (fRecursionCount > 0) closeSockets(); else delete this;
      // Note: The "fRecursionCount" test is for a pathological situation where we reenter the event loop and get called recursively
      // while handling a command (e.g., while handling a "DESCRIBE", to get a SDP description).
      // In such a case we don't want to actually delete ourself until we leave the outermost call.
    }
}

CustomMediaServer::MediaClientSession::MediaClientSession(CustomMediaServer& ourServer, u_int32_t sessionId)
    : GenericMediaServer::ClientSession(ourServer, sessionId), _mediaServer(ourServer)
{

}

CustomMediaServer::MediaClientSession::~MediaClientSession()
{

}
