#include <JPEGVideoRTPSink.hh>
#include "MJpegMediaSource.hh"
#include "MJpegMediaSubsession.hh"


MJpegMediaSubsession*
MJpegMediaSubsession::createNew(UsageEnvironment& env, const char* fileName, Boolean reuseFirstSource)
{
    return new MJpegMediaSubsession(env, fileName, reuseFirstSource);
}

MJpegMediaSubsession::MJpegMediaSubsession(UsageEnvironment& env, const char* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource)
{

}

MJpegMediaSubsession::~MJpegMediaSubsession()
{

}

void MJpegMediaSubsession::seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes)
{
    setStreamSourceDuration(inputSource, streamDuration, numBytes);
}

void MJpegMediaSubsession::setStreamSourceDuration(FramedSource* inputSource, double streamDuration, u_int64_t& numBytes)
{

}

FramedSource*
MJpegMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    MJPEGVideoSource* frameSource = MJPEGVideoSource::createNew(envir(), 30);
    frameSource->feedFrameData(fFileName);
    return frameSource;
}

RTPSink*
MJpegMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char /* = rtpPayloadTypeIfDynamic */, FramedSource* /* = inputSource */)
{
    return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);
}

float MJpegMediaSubsession::duration() const
{
    return fFileDuration;
}
