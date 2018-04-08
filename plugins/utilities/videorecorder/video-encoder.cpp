#include "cssysdef.h"
#include "iutil/objreg.h"
#include "csgfx/imagemanipulate.h"

#include "video-encoder.h"

// these functions are necessary to redirect libavformat output to iFile
static int iFile_write(void *opaque, uint8_t *buf, int buf_size)
{
	iFile* file = (iFile*)opaque;
	return file->Write((char*)buf, buf_size);
}

static int64_t iFile_seek(void *opaque, int64_t offset, int whence)
{
	iFile* file = (iFile*)opaque;
	switch (whence)
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += file->GetPos();
		break;
	case SEEK_END:
		offset += file->GetSize();
		break;
	default:
		return AVERROR(EINVAL);
	}
	if (!file->SetPos(offset))
		return AVERROR(EIO);
	return file->GetPos();
}

csString VideoEncoder::GetError()
{
  MutexScopedLock lock(mutex);
  csString Str = error;
  return Str;
}

void VideoEncoder::SetError(const char* fmt, ...)
{
  MutexScopedLock lock(mutex);
  va_list args;
  va_start (args, fmt);
  error.FormatV(fmt, args);
  va_end (args);

  recording = false;
}

VideoEncoder::VideoEncoder(
	           csRef<iVFS> VFS, iConfigFile* config,
	           const csString& filename,
		       int width, int height, int framerate,
			   uint8 channels, int freq)
{
  this->width = width;
  this->height = height;
  this->framerate = framerate;
  this->channels = channels;
  this->freq = freq;
  this->filename = filename;

  this->config = config;
  this->VFS = VFS;

  recording = false;

  avFormat = NULL;
  avContainer = NULL;
  videoStream = NULL;
  video = NULL;
  videoFrame = NULL;
  videoCodec = NULL;
  audioStream = NULL;
  audio = NULL;
  audioFrame = NULL;
  audioCodec = NULL;
  
  int queueLength = config->GetInt("VideoRecorder.Queue", 6);

  videoCodecName = config->GetStr("VideoRecorder.VideoCodec", "libx264");
  audioCodecName = config->GetStr("VideoRecorder.AudioCodec", "no");

  videoQuality = config->GetFloat("VideoRecorder.VideoQuality", -1);
  videoBitrate = config->GetInt("VideoRecorder.VideoBitRate", 3000000);

  audioQuality = config->GetFloat("VideoRecorder.AudioQuality", -1);
  audioBitrate = config->GetInt("VideoRecorder.AudioBitRate", 100000);
  
  ThreadPriority priority;
  const char* prio = config->GetStr("VideoRecorder.Prioriry", "normal");
  if (strcmp(prio, "low") == 0)
	  priority = THREAD_PRIO_LOW;
  else if (strcmp(prio, "normal") == 0)
	  priority = THREAD_PRIO_NORMAL;
  else if (strcmp(prio, "high") == 0)
	  priority = THREAD_PRIO_HIGH;
  else
	  priority = THREAD_PRIO_NORMAL;

  // allocate buffers for holding color planes
  Y.SetSize(width*height);
  U.SetSize(width*height/4); // because of YUV 4:2:0 format, U and V are 2x times smaller in each direction
  V.SetSize(width*height/4); // (4x total)

  videoQueue.SetSize(queueLength);
  videoQueueTicks.SetSize(queueLength);
 
  videoQueueWritten = 0;
  videoQueueRead = 0;
  audioQueueWritten = 0;
  audioQueueRead = 0;
  
  recording = true;
  recordAudio = false;

  encodingThread.AttachNew (new Thread(this));
  encodingThread->SetPriority(priority);
  encodingThread->Start();
}

VideoEncoder::~VideoEncoder()
{
  // free everything
  if (video)
     avcodec_close(video);
  if (audio)
     avcodec_close(audio);
  av_free(videoFrame);
  av_free(audioFrame);
  avformat_free_context(avContainer);
}

bool VideoEncoder::NeedFrame()
{
  // check if queue is not full
  MutexScopedLock lock (mutex);
  return videoQueueWritten - videoQueueRead < videoQueue.GetSize();
}

void VideoEncoder::Wait()
{
  MutexScopedLock lock (mutex);
  if (videoQueueWritten - videoQueueRead < videoQueue.GetSize())
	return;
  eventReady.Wait(mutex);
}

void VideoEncoder::AddFrame(csMicroTicks Time, csRef<csImageMemory> Frame)
{
  {
    // Make sure we lock the queue before trying to access it
    MutexScopedLock lock (mutex);
	
    // Put image in queue
    videoQueue.Put(videoQueueWritten % videoQueue.GetSize(), Frame);
	videoQueueTicks[videoQueueWritten % videoQueue.GetSize()] = Time;
	videoQueueWritten++;
  }
  eventWakeUp.NotifyOne(); // wake up encoding thread if it is sleeping
}

void VideoEncoder::Stop()
{
  {
    MutexScopedLock lock (mutex);
    recording = false;
  }
  // wake up encoding thread if it is sleeping
  eventWakeUp.NotifyOne();

  // wait for encoding thread to finish and release it
  encodingThread->Wait();
  encodingThread.Invalidate();
}

// try to start video recording, returns false if something goes wrong
bool VideoEncoder::TryStart()
{
  // find format
  avFormat = av_guess_format(NULL, filename.GetData(), NULL);
  if (!avFormat)
  {
    SetError("Format for given extension was not found");
    return false;
  }

  // allocate the output media context
  avContainer = avformat_alloc_context();
  if (!avContainer)
  {
    SetError("Could not allocate output context");
    return false;
  }
  avContainer->oformat = avFormat;

  strncpy(avContainer->filename, filename.GetData(), sizeof(avContainer->filename));

  // find codecs
  videoCodec = avcodec_find_encoder_by_name(videoCodecName);
  audioCodec = avcodec_find_encoder_by_name(audioCodecName);

  if (!videoCodec)
  {
    SetError("Video codec %s was not found", videoCodecName.GetData());
    return false;
  }
    
  if (!InitVideoStream())
	return false;

  recordAudio = false;
  if (audioCodec && channels) // we can proceed without audio
	  recordAudio = InitAudioStream();
  if (GetError() != "")
	  return false;

  // write format info to log
  av_dump_format(avContainer, 0, avContainer->filename, 1);

  // open the output file, if needed
  if (!(avFormat->flags & AVFMT_NOFILE))
  {
    file = VFS->Open (filename, VFS_FILE_WRITE | VFS_FILE_UNCOMPRESSED);
    if (!file)
    {
      SetError("Couldn't open file for recording - %s", filename.GetData());
      return false;
    }
	// we are redirecting writing to iFile
	avContainer->pb = avio_alloc_context(writeBuffer, WRITE_BUFFER_SIZE,
		1, // write flag (true)
		(iFile*)file, // user data pointer
		NULL, // read function
		iFile_write, // write function
		iFile_seek // seek function
	);
    if (!avContainer->pb)
    {
      SetError("avio_alloc_context failed");
	  return false;
	}
  }

  // write the stream header, if any
  avformat_write_header(avContainer, NULL);

  return true;
}

//initialize output video stream
bool VideoEncoder::InitVideoStream()
{
  videoStream = avformat_new_stream(avContainer, videoCodec);
  if (!videoStream)
  {
     SetError("Could not allocate video stream");
     return false;
  }

  video = videoStream->codec;

  avcodec_get_context_defaults3(video, videoCodec);
  video->codec_id = videoCodec->id;

  // put parameters
  // resolution must be a multiple of two
  video->width  = width  & ~1; // make even (dimensions should be even)
  video->height = height & ~1; // make even
  /* time base: this is the fundamental unit of time (in seconds) in terms
     of which frame timestamps are represented. for fixed-fps content,
     timebase should be 1/framerate and timestamp increments should be
     identically 1. */
  video->time_base.den = framerate;
  video->time_base.num = 1;
  video->pix_fmt = PIX_FMT_YUV420P;

  video->bit_rate = videoBitrate;
  if (videoQuality >= 0)
  {
    video->flags |= CODEC_FLAG_QSCALE;
    video->global_quality = videoQuality*FF_QP2LAMBDA;
  }

  // some formats want stream headers to be separate
  if (avFormat->flags & AVFMT_GLOBALHEADER)
      video->flags |= CODEC_FLAG_GLOBAL_HEADER;

  // set codec specific options
  AVDictionary* dict = NULL;
  csString codecName = videoCodec->name;
  csRef<iConfigIterator> option (config->Enumerate("VideoRecorder." + codecName + "."));
  while (option->Next())
    av_dict_set(&dict, option->GetKey(true), option->GetStr(), 0);

  // open the codec
  if (avcodec_open2(video, videoCodec, &dict) < 0)
  {
	SetError("Could not open video codec %s", videoCodec->long_name);
    return false;
  }

  av_dict_free(&dict);

  videoFrame = avcodec_alloc_frame();
  if (!videoFrame)
  {
    SetError("Could not allocate video frame");
    return false;
  }

  videoFrame->linesize[0] = width; // Y
  videoFrame->linesize[1] = width/2; // U, width is divided by 2 because of 4:2:0 chroma subsampling
  videoFrame->linesize[2] = width/2; // V
  videoFrame->linesize[3] = 0;
  return true;
}

// initialize output audio stream
bool VideoEncoder::InitAudioStream()
{
  audioFrame = avcodec_alloc_frame();
  if (!audioFrame)
    return false;

  audioStream = avformat_new_stream(avContainer, audioCodec);
  if (!audioStream)
     return false;
  audioStream->id = 1;

  audio = audioStream->codec;

  avcodec_get_context_defaults3(audio, audioCodec);
  audio->codec_id = audioCodec->id;
   
  audio->sample_fmt = AV_SAMPLE_FMT_S16;
  // find known sample format  
  if (audioCodec->sample_fmts)
  {
	bool s16 = false, s16p = false, flt = false, fltp = false;
	// let's see what formats this codec supports
	for (const AVSampleFormat* pFmt = audioCodec->sample_fmts; *pFmt != AV_SAMPLE_FMT_NONE; pFmt++)
	{
	  if (*pFmt == AV_SAMPLE_FMT_S16)
		s16 = true;
	  if (*pFmt == AV_SAMPLE_FMT_S16P)
		s16p = true;
	  if (*pFmt == AV_SAMPLE_FMT_FLT)
		flt = true;
	  if (*pFmt == AV_SAMPLE_FMT_FLTP)
		fltp = true;
	}
	// select sample format
	if (s16)
	  audio->sample_fmt = AV_SAMPLE_FMT_S16;
	else if (s16p)
	  audio->sample_fmt = AV_SAMPLE_FMT_S16P;
	else if (flt)
	  audio->sample_fmt = AV_SAMPLE_FMT_FLT;
	else if (fltp)
	  audio->sample_fmt = AV_SAMPLE_FMT_FLTP;
  }

  // put audio parameters
  audio->sample_rate = freq;
  audio->channels = channels;
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 26, 0)
  if (channels == 2)
	audio->channel_layout = AV_CH_LAYOUT_STEREO;
  else
	audio->channel_layout = AV_CH_LAYOUT_MONO;
#else
  audio->channel_layout = av_get_default_channel_layout(channels);
#endif

  audio->bit_rate = audioBitrate;
  if (audioQuality >= 0)
  {
    audio->flags |= CODEC_FLAG_QSCALE;
    audio->global_quality = audioQuality*FF_QP2LAMBDA;
  }

  // some formats want stream headers to be separate
  if (avFormat->flags & AVFMT_GLOBALHEADER)
      audio->flags |= CODEC_FLAG_GLOBAL_HEADER;

  // set codec specific options
  AVDictionary* dict = NULL;
  csString codecName = audioCodec->name;
  csRef<iConfigIterator> option (config->Enumerate("VideoRecorder." + codecName + "."));
  while (option->Next())
    av_dict_set(&dict, option->GetKey(true), option->GetStr(), 0);

  // open the codec
  if (avcodec_open2(audio, audioCodec, &dict) < 0)
  {
	// we already added new stream to our file, but now there is error, so now we can't simply ignore audio,
	// abort with error instead.
	SetError("Could not open audio codec %s", audioCodec->long_name);
    return false;
  }

  av_dict_free(&dict);
  
#if LIBAVCODEC_VERSION_MAJOR >= 54
  if (audioCodec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
#else
  if (audio->frame_size == 0)
#endif
    numSamples = 2048;
  else
    numSamples = audio->frame_size;

  {
    MutexScopedLock lock (mutex);
	audioQueue.SetSize(numSamples*channels*64);
  }
 
  return true;
}

void VideoEncoder::Run()
{    
  // initialize libav* and register all codecs and formats
  av_register_all();

  if (!TryStart())
	return;

  while (true)
  {
    csRef<iImage> screenshot;
    csMicroTicks time;

	eventReady.NotifyAll();
    {
      // Make sure we lock the queue before trying to access it
      MutexScopedLock lock (mutex);
      // Wait until we have an image
      while (videoQueueRead >= videoQueueWritten && recording)
        eventWakeUp.Wait (mutex);

	  if (!recording)
		break;

      // Get image
      screenshot = videoQueue[videoQueueRead % videoQueue.GetSize()];
	  time = videoQueueTicks[videoQueueRead % videoQueue.GetSize()];
    }
	SaveFrame(screenshot, time);
	SaveAudio();
	{
	  if (error != "")
		return;
      MutexScopedLock lock (mutex);
	  videoQueueRead++;
	}
  }

  // encode remaining frames
  {
	  MutexScopedLock lock (mutex);
	  while (videoQueueRead < videoQueueWritten)
	  {
		csRef<iImage> screenshot = videoQueue[videoQueueRead % videoQueue.GetSize()];
		SaveFrame(screenshot, videoQueueTicks[videoQueueRead % videoQueue.GetSize()]);
		videoQueueRead++;
	  }
  }

  // output buffered frames
  if (videoCodec && videoCodec->capabilities & CODEC_CAP_DELAY)
    while (WriteFrame(NULL, 0));
  
  if (recordAudio && audioCodec && audioCodec->capabilities & CODEC_CAP_DELAY)
    while (WriteAudio(NULL));

  // write the trailer, if any.
  av_write_trailer(avContainer);
  file->Flush();
}

bool VideoEncoder::WriteFrame(AVFrame* frame, csMicroTicks time)
{
  AVPacket Packet;
  av_init_packet(&Packet);
  Packet.data = NULL;
  Packet.size = 0;
 
  videoFrame->pts = time*framerate/1000000;

  if (avFormat->flags & AVFMT_RAWPICTURE)
  {
    // raw video case
    Packet.flags |= AV_PKT_FLAG_KEY;
    Packet.stream_index = videoStream->index;
    Packet.data = (uint8_t*)frame;
    Packet.size = sizeof(AVPicture);

    if (av_interleaved_write_frame(avContainer, &Packet) != 0)
	{
      SetError("Error while writing video frame");
	  return false;
    }
    return false;
  }
  else
  {
#if LIBAVCODEC_VERSION_MAJOR >= 54
    int got_packet;
    if (avcodec_encode_video2(video, &Packet, frame, &got_packet) < 0)
	{
      SetError("avcodec_encode_video2 failed");
	  return false;
	}
    if (!got_packet) // this is ok, some codecs delay output
      return false;
		
    if (Packet.pts != AV_NOPTS_VALUE)
       Packet.pts = av_rescale_q(Packet.pts, video->time_base, videoStream->time_base);
    if (Packet.dts != AV_NOPTS_VALUE)
       Packet.dts = av_rescale_q(Packet.dts, video->time_base, videoStream->time_base);
#else 
    Packet.size = avcodec_encode_video(video, outBuffer, OUT_BUFFER_SIZE, frame);
    if (Packet.size < 0)
	{
      SetError("avcodec_encode_video failed");
	  return false;
	}
    if (Packet.size == 0) // this is ok
      return false;

    if (video->coded_frame->pts != AV_NOPTS_VALUE)
      Packet.pts = av_rescale_q(video->coded_frame->pts, video->time_base, videoStream->time_base);
    if (video->coded_frame->key_frame)
      Packet.flags |= AV_PKT_FLAG_KEY;
    Packet.data = outBuffer;
#endif
    // write the compressed frame in the media file
    Packet.stream_index = videoStream->index;
    if (av_interleaved_write_frame(avContainer, &Packet) != 0)
    {
      SetError("Error while writing video frame");
	  return false;
	}      
    return true;
  }
}

// Convert RGB to YUV 4:2:0 and encode video frame
void VideoEncoder::SaveFrame(csRef<iImage>& img, csMicroTicks time)
{
  // rescale image if necessary
  if (img->GetWidth() != width || img->GetHeight() != height)
    img = csImageManipulate::Rescale (img, width, height);

  unsigned char* data = (unsigned char*)img->GetImageData();
  
  int pitch = width*4;

  // Y
  for (int i = 0; i < height; i++)
  for (int j = 0; j < width; j++)
  {
	int k = height - i - 1; // invert image upside-down
	uint8_t* p = data + k*pitch + j*4;
	Y[i*width + j] = 16 + ((16828*p[2] + 33038*p[1] + 6416*p[0]) >> 16);
  }
  // U and V
  for (int i = 0; i < height/2; i++)
  for (int j = 0; j < width/2; j++)
  {
	int k = height/2 - i - 1; // invert image upside-down
	uint8_t* p1 = data + 2*k*pitch + 2*j*4;
	uint8_t* p2 = data + 2*k*pitch + 2*(j+1)*4;
	uint8_t* p3 = data + (2*k+1)*pitch + 2*j*4;
	uint8_t* p4 = data + (2*k+1)*pitch + (2*j+1)*4;
	int r = p1[0] + p2[0] + p3[0] + p4[0];
	int g = p1[1] + p2[1] + p3[1] + p4[1];
	int b = p1[2] + p2[2] + p3[2] + p4[2];
	U[i*width/2 + j] = 128 + ((-2428*r - 4768*g + 7196*b) >> 16);
	V[i*width/2 + j] = 128 + (( 7196*r - 6026*g - 1170*b) >> 16);
  }

  videoFrame->data[0] = &Y[0];
  videoFrame->data[1] = &U[0];
  videoFrame->data[2] = &V[0];
  WriteFrame(videoFrame, time);
}

bool VideoEncoder::WriteAudio(AVFrame* frame)
{
  if (!recordAudio)
    return false;

  AVPacket Packet = { 0 };
  av_init_packet(&Packet);

  int got_packet;
  if (avcodec_encode_audio2(audio, &Packet, frame, &got_packet) != 0)
  {
	SetError("avcodec_encode_audio2 failed");
	return false;
  }

  if (!got_packet) // this is ok, some codecs delay output
	return false;

  // Write the compressed frame to the media file.
  Packet.stream_index = audioStream->index;
  if (av_interleaved_write_frame(avContainer, &Packet) != 0)
  {
    SetError("Error while writing video frame");
	return false;
  }      
  return true;
}

void VideoEncoder::SaveAudio()
{
  if (!recordAudio)
    return;

  // read current queue status
  int read, written;  
  {
    MutexScopedLock lock (mutex);
	read = audioQueueRead;
	written = audioQueueWritten;
  }

  while (read + numSamples*channels <= written)
  {
    AVPacket Packet = { 0 };
    av_init_packet(&Packet);

    audioFrame->nb_samples = numSamples;

	void* buffer = (uint8_t*)&audioQueue[read % audioQueue.GetSize()];
	int bufSize = numSamples*channels*sizeof(uint16);
	
	// convert to float if necessary (map from [-2^15; 2^15 - 1] to [-1.0, 1.0])
    if (audio->sample_fmt == AV_SAMPLE_FMT_FLT || audio->sample_fmt == AV_SAMPLE_FMT_FLTP)
	{
	  audioBufferFlt.SetSize(numSamples*channels);
	  for (int i = 0; i < numSamples*channels; i++)
		audioBufferFlt[i] = audioQueue[read % audioQueue.GetSize() + i] / 32768.0f;
	  buffer = &audioBufferFlt[0];
	  bufSize = numSamples*channels*sizeof(float);
	}

	avcodec_fill_audio_frame(audioFrame, audio->channels, audio->sample_fmt, (uint8_t*)buffer, bufSize, 1);
	read += numSamples*channels;

	WriteAudio(audioFrame);
  }

  // write new queue status
  {
    MutexScopedLock lock (mutex);
	audioQueueRead = read;
  }
}

void VideoEncoder::DeliverSoundData (const csSoundSample *SampleBuffer, size_t NumSamples)
{
  // read current queue status
  int /*read,*/ written;  
  {
    if (!recordAudio)
	  return;
    MutexScopedLock lock (mutex);
	//read = audioQueueRead;
	written = audioQueueWritten;
  }

  if (audio->sample_fmt == AV_SAMPLE_FMT_S16P || audio->sample_fmt == AV_SAMPLE_FMT_FLTP)
  {
	// codec wants data in planar format (that is - first numSamples for first channel, then for second)
	// we need to do hard job for this conversion
	for (uint i = 0; i < NumSamples; i++)
	  for  (uint j = 0; j < channels; j++)
	  {
		int s = (written + i*channels + j) % audioQueue.GetSize();
		int N = numSamples*channels;
		int n = s / N;
		int k = s % N;
        audioQueue[n*N + (k % channels)*numSamples + k/channels] = SampleBuffer[i + NumSamples*j] >> 16;
	  }
  }
  else
  {
	// codec wants data in interleaved format - one sample for left channel, one sample for right channel, etc.
	// this is simpler
	for (uint i = 0; i < NumSamples; i++)
	  for  (uint j = 0; j < channels; j++)
        audioQueue[(written + i*channels + j) % audioQueue.GetSize()] = SampleBuffer[i + NumSamples*j] >> 16;
  }

  // write new queue status
  {
    MutexScopedLock lock (mutex);
	audioQueueWritten += channels*NumSamples;
  }
}

void VideoEncoder::DeliverSoundData (const uint16 *SampleBuffer, size_t NumSamples)
{
  // read current queue status
  int /*read,*/ written;  
  {
    if (!recordAudio)
	  return;
    MutexScopedLock lock (mutex);
	//read = audioQueueRead;
	written = audioQueueWritten;
  }

  if (audio->sample_fmt == AV_SAMPLE_FMT_S16P || audio->sample_fmt == AV_SAMPLE_FMT_FLTP)
  {
	// codec wants data in planar format (that is - first numSamples for first channel, then for second)
	// we need to do hard job for this conversion
	for (uint i = 0; i < NumSamples; i++)
	  for  (uint j = 0; j < channels; j++)
	  {
		int s = (written + i*channels + j) % audioQueue.GetSize();
		int N = numSamples*channels;
		int n = s / N;
		int k = s % N;
        audioQueue[n*N + (k % channels)*numSamples + k/channels] = SampleBuffer[i*channels + j];
	  }
  }
  else
  {
	// codec wants data in interleaved format - one sample for left channel, one sample for right channel, etc.
	// this is simpler
	for (uint i = 0; i < NumSamples; i++)
	  for  (uint j = 0; j < channels; j++)
        audioQueue[(written + i*channels + j) % audioQueue.GetSize()] = SampleBuffer[i*channels + j];
  }

  // write new queue status
  {
    MutexScopedLock lock (mutex);
	audioQueueWritten += channels*NumSamples;
  }
}
