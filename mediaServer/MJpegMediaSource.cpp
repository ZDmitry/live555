#include "MJpegMediaSource.hh"
#include <InputFile.hh>
#include <algorithm>


MJPEGVideoSource*
MJPEGVideoSource::createNew(UsageEnvironment& env, unsigned timePerFrame)
{
    return new MJPEGVideoSource(env, timePerFrame);
}

void MJPEGVideoSource::feedFrameData(const char* fileName)
{
    FILE* fid = OpenInputFile(envir(), fileName);
    if (fid) {
        // Medium::close(newSource);
        u_int64_t fsize = GetFileSize(fileName, fid);

        const u_int8_t* fdata = new u_int8_t[fsize];

        SeekFile64(fid, 0L, SEEK_SET);
        fread((void*)fdata, sizeof(u_int8_t), fsize, fid);

        CloseInputFile(fid);

        feedFrameData(fdata, (size_t)fsize);

        delete[] fdata;
    }
}

void MJPEGVideoSource::feedFrameData(const u_int8_t* data, size_t len)
{
    if (!_frameData || _frameSize != len) {
        if (_frameData) delete[] _frameData;
        _frameData = new u_int8_t[len];
    }

    memcpy(_frameData, data, len);
    _frameSize = len;

    if(_jpeg->parse(_frameData, _frameSize) == 0) { // successful parsing
        _sourcePtr = (unsigned char*)_jpeg->scandata(_bytesToRead);
    }
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
    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if (_bytesToRead < fMaxSize) {
      fMaxSize = _bytesToRead;
    }

    // Set the 'presentation time' and 'duration' of this frame:
    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);
    } else {
      // Increment by the play time of the previous data:
      unsigned uSeconds = fPresentationTime.tv_usec + _lastPlayTime;
      fPresentationTime.tv_sec  += uSeconds / 1000000;
      fPresentationTime.tv_usec  = uSeconds % 1000000;
    }

    fDurationInMicroseconds = _lastPlayTime
        = (_lastPlayTime + _timePerFrame * 1000);

    unsigned numBytesRead = 0;
    if (_sourcePtr && _bytesToRead > 0) {
        memcpy(fTo, _sourcePtr, _bytesToRead);
        numBytesRead = fMaxSize;
        fFrameSize = _bytesToRead;
    }

    // fFrameSize += numBytesRead;
    // fTo += numBytesRead;
    // fMaxSize -= numBytesRead;
    // _sourcePtr += numBytesRead;
    // _bytesToRead -= numBytesRead;

    if (_bytesToRead == 0) {
        _sourcePtr = (unsigned char*)_jpeg->scandata(_bytesToRead);
    }

    // Switch to another task, and inform the reader that he has data:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)FramedSource::afterGetting, this);
}

size_t MJPEGVideoSource::fillFrameData(void *pto, void *pfrom, size_t len)
{
    unsigned int  jpegDataLen;
    unsigned char const* jpegData;
    if(_jpeg->parse((unsigned char*)_frameData, _frameSize) == 0) { // successful parsing
        jpegData = _jpeg->scandata(jpegDataLen);
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
