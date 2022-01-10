#ifndef MJPEGMEDIASUBSESSION_H
#define MJPEGMEDIASUBSESSION_H

#include <FileServerMediaSubsession.hh>


class MJpegMediaSubsession: public FileServerMediaSubsession
{
public:
  static MJpegMediaSubsession*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

private:
  MJpegMediaSubsession(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);
  virtual ~MJpegMediaSubsession();

private: // redefined virtual functions
  // virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
  virtual void seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes);
  virtual void setStreamSourceDuration(FramedSource* inputSource, double streamDuration, u_int64_t& numBytes);
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
  virtual float duration() const;

private:
  float fFileDuration; // in seconds

};

#endif // MJPEGMEDIASUBSESSION_H
