#include "MJpegMediaSource.hh"

#include <GroupsockHelper.hh>
#include <InputFile.hh>
#include <algorithm>


MJPEGVideoSource*
MJPEGVideoSource::createNew(UsageEnvironment& env, unsigned timePerFrame)
{
    return new MJPEGVideoSource(env, timePerFrame);
}

void MJPEGVideoSource::setupMediaSource(CustomMediaClient* source)
{
  _media = source;
}

u_int8_t MJPEGVideoSource::type()
{
    return (u_int8_t)_jpeg->type();
}

u_int8_t MJPEGVideoSource::qFactor()
{
    return (u_int8_t)_jpeg->qFactor();
}

u_int8_t MJPEGVideoSource::width()
{
    return (u_int8_t)_jpeg->width();
}

u_int8_t MJPEGVideoSource::height()
{
    return (u_int8_t)_jpeg->height();
}

const u_int8_t*
MJPEGVideoSource::quantizationTables(u_int8_t& precision, u_int16_t& length)
{
    precision = (u_int8_t)_jpeg->precision();
    return (const u_int8_t*)_jpeg->quantizationTables(length);
}

u_int16_t MJPEGVideoSource::restartInterval()
{
    return (u_int16_t)_jpeg->restartInterval();
}


void MJPEGVideoSource::doGetNextFrame()
{
    fDurationInMicroseconds = 1000000 / _timePerFrame;

    // Set the 'presentation time' and 'duration' of this frame:
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);
    } else {
      // Increment by the play time of the previous data:
      unsigned uSeconds = fPresentationTime.tv_usec + fDurationInMicroseconds;
      fPresentationTime.tv_sec  += uSeconds / 1000000;
      fPresentationTime.tv_usec  = uSeconds % 1000000;
    }

    _custom_frame frame = _media->dequeueFrame();
    if (frame.data && frame.size) {
      if (_frameData) delete[] _frameData;
      _frameData = (u_int8_t*)frame.data;
      _frameSize = frame.size;
      _bytesToRead = _frameSize;
    }

    if(_bytesToRead > fMaxSize) {
      fprintf(stderr, "WebcamJPEGDeviceSource::doGetNextFrame(): read maximum buffer size: %d bytes.  Frame may be truncated\n", fMaxSize);
    }

    if (_frameData && _frameSize > 0) {
      fFrameSize = fillFrameData(fTo, _frameData, (std::min)(_bytesToRead, fMaxSize));
    }

    // fFrameSize += numBytesRead;
    // fTo += numBytesRead;
    // fMaxSize -= numBytesRead;
    // _sourcePtr += numBytesRead;
    // _bytesToRead -= numBytesRead;

    // Switch to another task, and inform the reader that he has data:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)FramedSource::afterGetting, this);
}

size_t MJPEGVideoSource::fillFrameData(void* pto, void* pfrom, size_t len)
{
    unsigned char* dest   = (unsigned char*)pto;
    unsigned char* source = (unsigned char*)pfrom;

    unsigned int  jpegDataLen;
    unsigned char const* jpegData;
    if(_jpeg->parse(source, len) == 0) { // successful parsing
        jpegData = _jpeg->scandata(jpegDataLen);
        memcpy(dest, jpegData, jpegDataLen);
        return jpegDataLen;
    }
    return 0;
}

Boolean MJPEGVideoSource::isJPEGVideoSource() const
{
    return true;
}

MJPEGVideoSource::MJPEGVideoSource(UsageEnvironment &env, unsigned timePerFrame)
  : JPEGVideoSource(env), _lastPlayTime(0), _timePerFrame(timePerFrame), _bytesToRead(0),
    _jpeg(new JpegFrameParser()), _frameData(NULL), _sourcePtr(NULL), _frameSize(0)
{

}

MJPEGVideoSource::~MJPEGVideoSource()
{
    if (_jpeg) delete _jpeg;
    if (_frameData) delete[] _frameData;
}
