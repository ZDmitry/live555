#include <JPEGVideoRTPSink.hh>
#include "MJpegMediaSource.hh"
#include "MJpegMediaSubsession.hh"


MJpegMediaSubsession*
MJpegMediaSubsession::createNew(UsageEnvironment& env, CustomMediaClient* source, Boolean reuseFirstSource)
{
    return new MJpegMediaSubsession(env, source, reuseFirstSource);
}

MJpegMediaSubsession::MJpegMediaSubsession(UsageEnvironment& env, CustomMediaClient* source, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, "> Motion JPEG", reuseFirstSource), fMediaSource(source)
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
    frameSource->setupMediaSource(fMediaSource);
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
