#ifndef MJPEGMEDIASOURCE_HH
#define MJPEGMEDIASOURCE_HH

#include <JPEGVideoSource.hh>
#include "JpegFrameParser.hh"

#ifndef _CUSTOM_MEDIA_CLIENT_HH
#include "CustomMediaClient.hh"
#endif


class MJPEGVideoSource : public JPEGVideoSource
{
public:
  static MJPEGVideoSource*
  createNew(UsageEnvironment& env, unsigned timePerFrame);

  void setupMediaSource(CustomMediaClient* source);

protected:
  virtual u_int8_t type();
  virtual u_int8_t qFactor();
  virtual u_int8_t width();   // # pixels/8 (or 0 for 2048 pixels)
  virtual u_int8_t height();  // # pixels/8 (or 0 for 2048 pixels)

  virtual u_int8_t const*
  quantizationTables(u_int8_t& precision, u_int16_t& length);

  virtual u_int16_t restartInterval();
  virtual void      doGetNextFrame();

private:
  virtual Boolean   isJPEGVideoSource() const;
  size_t  fillFrameData(void *pto, void *pfrom, size_t len);

protected:
  MJPEGVideoSource(UsageEnvironment& env, unsigned timePerFrame);
  virtual ~MJPEGVideoSource();

private:
  unsigned _lastPlayTime; // useconds
  unsigned _timePerFrame;
  unsigned _bytesToRead;

  JpegFrameParser*   _jpeg;
  CustomMediaClient* _media;

  u_int8_t* _frameData;
  u_int8_t* _sourcePtr;
  size_t    _frameSize;
};

#endif // MJPEGMEDIASOURCE_HH
