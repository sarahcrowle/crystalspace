#ifndef __CS_VIDEOENCODER_H__
#define __CS_VIDEOENCODER_H__

#include "iutil/vfs.h"	
#include "csutil/threading/thread.h"
#include "csutil/csstring.h"
#include "csutil/cfgacc.h"
#include "csutil/array.h"
#include "csgfx/imagememory.h"
#include "isndsys/ss_structs.h"

extern "C"
{
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 2, 0)
  #include <libavutil/audioconvert.h>
#else
  #include <libavutil/channel_layout.h>
#endif
}

using namespace CS::Threading;

class VideoEncoder : public Runnable
{
  static const int WRITE_BUFFER_SIZE = 4*1024;
#if LIBAVCODEC_VERSION_MAJOR < 54
  static const int OUT_BUFFER_SIZE = 500000;
#endif

  csString filename;
  csString videoCodecName;
  csString audioCodecName;

  // video parameters
  int width, height;
  int framerate;

  // sound parameters
  uint8 channels; // 1 or 2 (0 - disable audio)
  int freq;
  int numSamples; // number of samples that should be delivered to codec per call

  // if quality is not set (negative) then bitrate is used
  float videoQuality; // quality scale is codec depended
  float audioQuality;
  int videoBitrate; // bits per second
  int audioBitrate;

  bool recordAudio;
  
  csRef<iVFS> VFS;
  csRef<iFile> file;

  // VideoRecorder config (we need to read codec specific options)
  iConfigFile* config;
  
  // libavcodec, libavformat data
  AVOutputFormat* avFormat; // format description
  AVFormatContext* avContainer; // current state of writing file in specified format
  AVStream* audioStream;
  AVStream* videoStream;
  AVFrame* audioFrame;
  AVFrame* videoFrame;
  AVCodec* audioCodec; // audio codec description
  AVCodec* videoCodec; // video codec description
  AVCodecContext* audio; // current state of audio encoding
  AVCodecContext* video; // current state of video encoding

  // color planes
  csArray<unsigned char> Y, U, V; // also called Y, Cb, Cr
  // write buffer (it is used for redirecting libavformat output to iFile)
  unsigned char writeBuffer[WRITE_BUFFER_SIZE];
   
#if LIBAVCODEC_VERSION_MAJOR < 54
  unsigned char outBuffer[OUT_BUFFER_SIZE];
#endif

  csArray<float> audioBufferFlt; // buffer for converting audio samples from int16 to float (required for some codecs)
  
  // set in encoding thread to signal error (this variable is protected by mutex)
  csString error;

  // the only purpose of this cariable is to notify encoding thrad that it should stop
  // (this variable is protected by mutex)
  bool recording;

  csRefArray<csImageMemory> videoQueue; // curcullar buffer
  csArray<csMicroTicks> videoQueueTicks; // timestamp for each frame
  uint videoQueueWritten, videoQueueRead;
  
  csArray<int16> audioQueue; // curcullar buffer
  uint audioQueueWritten, audioQueueRead;
 
  Mutex mutex; // mutex for queues, `error` and `recording` variables
  Condition eventWakeUp; // this event is dispatched by main thread when new frame was put into video queue or video is over
  Condition eventReady; // this event is dispatched by encoding thread when it is ready for new frames
   
  csRef<Thread> encodingThread;

  // internal functions (called in encoding thread) 
  bool TryStart();
  bool InitVideoStream();
  bool InitAudioStream();
  bool WriteFrame(AVFrame* frame, csMicroTicks time);
  void SaveFrame(csRef<iImage>& image, csMicroTicks time);
  bool WriteAudio(AVFrame* frame);
  void SaveAudio();

  void SetError(const char* fmt, ...);

  //-- Runnable
  void Run ();
  const char* GetName () const
  {
    return "Video encoder";
  }

public:
	
  VideoEncoder(csRef<iVFS> VFS, iConfigFile* config,
	           const csString& filename,
		       int width, int height, int framerate,
			   uint8 channels, int freq);
  ~VideoEncoder();
   
  /// check if queue is not full
  bool NeedFrame();
  
  /// wait until thread is ready for encoding (this forces NeedFrame() to return true)
  void Wait();

  /// you should ensure that NeedFrame() returns true or you should call Wait() before calling this function
  void AddFrame(csMicroTicks Time, csRef<csImageMemory> Frame);
  
  // function for sound recording (passed from iSndSysSoftwareOutputFilter)
  // samples are in planar format
  void DeliverSoundData (const csSoundSample *SampleBuffer, size_t Frames);

  // function for sound recording
  // samples are interleaved
  void DeliverSoundData (const uint16 *SampleBuffer, size_t Frames);

  // stop encoding and wait for it to finish
  void Stop();
  
  csString GetError();
};

#endif // __CS_VIDEOENCODER_H__