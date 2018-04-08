/*
Copyright (C) 2010 by Alin Baciu

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "csutil/refarr.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "ivideo/texture.h"

#include "theoravideomedia.h"

SCF_IMPLEMENT_FACTORY (csTheoraVideoMedia)


csTheoraVideoMedia::csTheoraVideoMedia (iBase* parent)
: scfImplementationType (this, parent),
  _object_reg (0), _name (nullptr)
{
}

csTheoraVideoMedia::~csTheoraVideoMedia ()
{
  delete _name;
}

void csTheoraVideoMedia::CleanMedia ()
{
  ogg_stream_clear (&_streamState);

  if (_decodersStarted)
  {
    th_decode_free (_decodeControl);
  }

  th_comment_clear (&_streamComments);
  th_info_clear (&_streamInfo);
  th_setup_free (_setupInfo);
}

bool csTheoraVideoMedia::Initialize (iObjectRegistry* r)
{
  _object_reg = r;

  // Initialize the decoders
  if (_theora_p)
  {
    // Clear the theora state in case it contains previous data
    _decodeControl = nullptr;

    // Initialize the decoders and print the info on the stream
    _decodeControl = th_decode_alloc (&_streamInfo,_setupInfo);

    if (_streamInfo.aspect_numerator == 0 or _streamInfo.aspect_denominator == 0)
      _aspectRatio = 1.0;
    else
      _aspectRatio = (float)_streamInfo.aspect_numerator / _streamInfo.aspect_denominator;

    if (_streamInfo.fps_numerator == 0 or _streamInfo.fps_denominator == 0)
      _FPS = -1.0;
    else
      _FPS = (double)_streamInfo.fps_numerator / _streamInfo.fps_denominator;

    _decodersStarted=true;
    _videobuf_granulepos=-1;
    _videobufTime=0;
    _frameToSkip=-1;
    _cacheSize=1;
  }
  else
  {
    // Tear down the partial theora setup
    th_info_clear (&_streamInfo);
    th_comment_clear (&_streamComments);

    _decodersStarted=false;
  }

  return 0;
}

const char* csTheoraVideoMedia::GetName () const
{
  return _name;
}

const char* csTheoraVideoMedia::GetType () const
{
  return "TheoraVideo";
}

unsigned long csTheoraVideoMedia::GetFrameCount() const
{
  return _frameCount;
}

float csTheoraVideoMedia::GetDuration () const
{
  return _length;
}

iTextureHandle* csTheoraVideoMedia::GetVideoTarget ()
{
  // We want "texture" to point to its internal representation in the stream
  return _texture;
}

double csTheoraVideoMedia::GetPosition () const
{
  return _videobufTime;
}

bool csTheoraVideoMedia::Update ()
{
  if (_cache.GetSize() >= _cacheSize)
    return false;

  _videobufReady = false;

  while (_theora_p && !_videobufReady)
  {
    if (ogg_stream_packetout (&_streamState,&_oggPacket) > 0)
    {
      if (th_decode_packetin (_decodeControl,&_oggPacket,&_videobuf_granulepos) >= 0)
      {
        _videobufTime = th_granule_time (_decodeControl,_videobuf_granulepos);

        if (th_granule_frame (_decodeControl,_videobuf_granulepos) < _frameToSkip)
        {
          return false;
        }
        else
        {
          _videobufReady = true;
          _frameToSkip = -1;

          th_decode_ycbcr_out (_decodeControl,_currentYUVBuffer);
          Convert ();
        }
      }
    }
    else
      break;
  }

  if (!_videobufReady)
    return true;

  return false;
}

long csTheoraVideoMedia::SeekPage (long targetFrame, long frameCount, bool return_keyframe,
                                   ogg_sync_state *oy,unsigned long fileSize)
{
  if (frameCount == 0)
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR, QUALIFIED_PLUGIN_NAME,
              "The active video stream has a null frame count. Seeking not available.\n");
    return -1;
  }

  MutexScopedLock lock (_writeMutex);
  while (_isWrite)
    _isWriting.Wait (_writeMutex);

  // Reset Theora decoder
  _rgbBuff = nullptr;
  _cache.DeleteAll ();
  ogg_stream_reset (&_streamState);
  th_decode_free (_decodeControl);
  _decodeControl=th_decode_alloc (&_streamInfo,_setupInfo);

  // Init seeking interval
  float seekRatio = (float)targetFrame / frameCount;
  float frameSize = (float)fileSize / frameCount;
  int seek_min = std::max(0, (int)(fileSize * seekRatio - 10 * frameSize));
  int seek_max = targetFrame == 0 ?
    0 : std::min((int)(fileSize * seekRatio + 10 * frameSize), (int)fileSize);

  // Init data
  long frame;
  ogg_int64_t granule=0;
  bool fineseek=false;
  ogg_page og;

  for (int i=0; i<100; i++)
  {
    // Reset the internal counters of the ogg_sync_state struct
    // that tracks the synchronization of the current page.
    ogg_sync_reset (oy);

    // Goto searching position in the stream
    fseek (_infile,(seek_min+seek_max)/2,SEEK_SET);

    // Try to synchronize to the next page in the bitstream
    memset (&og, 0, sizeof (ogg_page));
    ogg_sync_pageseek (oy,&og);

    while (true)
    {
      // Try to read a new page.
      int ret = ogg_sync_pageout ( oy, &og );

      // Check if the page was synced and returned.
      if (ret == 1)
      {
        // Get the unique serial number for the logical bitstream of this page.
        int serno = ogg_page_serialno (&og);
        if (serno == _streamState.serialno)
        {
          // Get the last granular position of the decoded data contained in the page.
          granule = ogg_page_granulepos (&og);
          if (granule >= 0)
          {
            // Convert granule position to an absolute frame index.
            frame = (long) th_granule_frame (_decodeControl,granule);
            if ((targetFrame == 0 and frame == 0) or
                (frame < targetFrame-1 && targetFrame-frame < 10))
            {
              fineseek = true;
              if (!return_keyframe) break;
            }

            if (fineseek && frame >= targetFrame)
              break;

            if (fineseek)
              continue;

            if (seek_max - seek_min <= 1)
            {
              fineseek = true;
              break;
            }

            if (targetFrame-1 > frame)
              seek_min = (seek_min+seek_max)/2;
            else
              seek_max = (seek_min+seek_max)/2;

            break;
          }
        }
      }
      else  // Stream has not yet captured sync.
      {
        char *buffer = ogg_sync_buffer (oy, 4096);
        int bytesRead = fread (buffer,1,4096,_infile);
        if (bytesRead == 0) break;
        ogg_sync_wrote ( oy, bytesRead );
      }
    }

    if (fineseek) break;
  }

  // Add a complete page to the bitstream.
  ogg_stream_pagein (&_streamState,&og);

  // Set the granule position.
  th_decode_ctl (_decodeControl,TH_DECCTL_SET_GRANPOS,&granule,sizeof (granule));

  // Make sure we skip to the keyframe we need
  if (return_keyframe)
  {
    _frameToSkip = targetFrame;

    return (long) (granule >> _streamInfo.keyframe_granule_shift);
  }

  return -1;
}

void csTheoraVideoMedia::InitializeStream (const char* name, ogg_stream_state &state, th_info &info,
                                           th_comment &comments, th_setup_info *setupInfo, FILE *source,
                                           iTextureManager* texManager)
{
  _name = new char[strlen (name)];
  strcpy(_name, name);
  memcpy (&_streamState,&state,sizeof (state));
  memcpy (&_streamInfo,&info,sizeof (info));
  memcpy (&_streamComments,&comments,sizeof (comments));
  memcpy (&_setupInfo,&setupInfo,sizeof (setupInfo));
  _theora_p=1;

  _decodersStarted = false;
  _infile = source;

  // Create the buffers needed for double buffering
  csRef<iTextureHandle> tex1 = texManager->CreateTexture
    (_streamInfo.pic_width, _streamInfo.pic_height, 0, csimg2D, "rgb8",
     CS_TEXTURE_2D | CS_TEXTURE_NPOTS);
  _buffers.Push (tex1);

  csRef<iTextureHandle> tex2 = texManager->CreateTexture
    (_streamInfo.pic_width, _streamInfo.pic_height, 0, csimg2D, "rgb8",
     CS_TEXTURE_2D | CS_TEXTURE_NPOTS);
  _buffers.Push (tex2);

  _activeBuffer = 1;

  _texture = _buffers[0];

  // Initialize the LUTs
  double scale = 1L << 8, temp;

  for (int i=0; i<256; i++)
  {
    temp = i - 128;

    Ylut[i] = (int) ( (1.164 * scale + 0.5) * (i - 16));
    RVlut[i] = (int) ( (1.596 * scale + 0.5) * temp);		//Calc R component
    GUlut[i] = (int) ( (0.391 * scale + 0.5) * temp);		//Calc G u & v components
    GVlut[i] = (int) ( (0.813 * scale + 0.5) * temp);
    BUlut[i] = (int) ( (2.018 * scale + 0.5) * temp);		//Calc B component
  }

  _canSwap=false;
}

void csTheoraVideoMedia::DropFrame ()
{
  if (_cache.GetSize ()!=0)
  {
    MutexScopedLock lock (_writeMutex);

    cachedData pixels = _cache.PopTop ();
    delete pixels;
  }
}

void csTheoraVideoMedia::Convert ()
{
  int Y,U,V,R,G,B;

  // Luma plane offset
  int y_offset = (_streamInfo.pic_x&~1) + _currentYUVBuffer[0].stride * (_streamInfo.pic_y&~1);

  // 4:2:0 pixel format
  if (_streamInfo.pixel_fmt==TH_PF_420)
  {
    // Chroma is decimated by 2 in both the X and Y directions (4:2:0).
    // The Cb and Cr chroma planes are half the width and half the height of the luma plane.
    int uv_offset = (_streamInfo.pic_x/2) + (_currentYUVBuffer[1].stride) * (_streamInfo.pic_y/2);
    uint8* outputBuffer = new uint8[_streamInfo.pic_width*_streamInfo.pic_height*4];
    int k=0;

    for (ogg_uint32_t y = 0 ; y < _streamInfo.pic_height ; y++)
      for (ogg_uint32_t x = 0 ; x < _streamInfo.pic_width ; x++)
      {
        Y = (_currentYUVBuffer[0].data + y_offset  + _currentYUVBuffer[0].stride * (y)  )[x] ;
        U = (_currentYUVBuffer[1].data + uv_offset + _currentYUVBuffer[1].stride * (y/2))[x/2] ;
        V = (_currentYUVBuffer[2].data + uv_offset + _currentYUVBuffer[2].stride * (y/2))[x/2] ;

        R = (Ylut[Y] + RVlut[V])>>8;
        G = (Ylut[Y] - GUlut[U] - GVlut[V])>>8;
        B = (Ylut[Y] + BUlut[U])>>8;

        // Clamping the values here is faster than calling a function
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        outputBuffer[k] = (uint8)R;
        k++;
        outputBuffer[k] = (uint8)G;
        k++;
        outputBuffer[k] = (uint8)B;
        k++;
        outputBuffer[k] = 0xff;
        k++;
      }

      _currentPixels = outputBuffer;
  }
  // 4:2:2 pixel format
  else if (_streamInfo.pixel_fmt==TH_PF_422)
  {
    // Chroma is decimated by 2 in the X direction (4:2:2).
    // The Cb and Cr chroma planes are half the width of the luma plane, but full height.
    int uv_offset = (_streamInfo.pic_x/2) + (_currentYUVBuffer[1].stride) * (_streamInfo.pic_y);
    uint8 * outputBuffer = new uint8[_streamInfo.pic_width*_streamInfo.pic_height*4];
    int k=0;

    for (ogg_uint32_t y = 0 ; y < _streamInfo.pic_height ; y++)
      for (ogg_uint32_t x = 0 ; x < _streamInfo.pic_width ; x++)
      {
        Y = (_currentYUVBuffer[0].data + y_offset  + _currentYUVBuffer[0].stride * y)[x] ;
        U = (_currentYUVBuffer[1].data + uv_offset + _currentYUVBuffer[1].stride * y)[x/2] ;
        V = (_currentYUVBuffer[2].data + uv_offset + _currentYUVBuffer[2].stride * y)[x/2] ;

        R = (Ylut[Y] + RVlut[V])>>8;
        G = (Ylut[Y] - GUlut[U] - GVlut[V])>>8;
        B = (Ylut[Y] + BUlut[U])>>8;

        // Clamping the values here is faster than calling a function
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        outputBuffer[k] = (uint8)R;
        k++;
        outputBuffer[k] = (uint8)G;
        k++;
        outputBuffer[k] = (uint8)B;
        k++;
        outputBuffer[k] = 0xff;
        k++;
      }

      _currentPixels = outputBuffer;
  }
  // 4:4:4 pixel format
  else if (_streamInfo.pixel_fmt==TH_PF_444)
  {
    // No chroma decimation (4:4:4).
    // The Cb and Cr chroma planes are full width and full height.
    int uv_offset = (_streamInfo.pic_x) + (_currentYUVBuffer[1].stride) * (_streamInfo.pic_y);
    uint8 * outputBuffer = new uint8[_streamInfo.pic_width*_streamInfo.pic_height*4];
    int k=0;

    for (ogg_uint32_t y = 0 ; y < _streamInfo.pic_height ; y++)
      for (ogg_uint32_t x = 0 ; x < _streamInfo.pic_width ; x++)
      {
        Y = (_currentYUVBuffer[0].data + y_offset  + _currentYUVBuffer[0].stride * y)[x];
        U = (_currentYUVBuffer[1].data + uv_offset + _currentYUVBuffer[1].stride * y)[x] ;
        V = (_currentYUVBuffer[2].data + uv_offset + _currentYUVBuffer[2].stride * y)[x] ;

        R = (Ylut[Y] + RVlut[V])>>8;
        G = (Ylut[Y] - GUlut[U] - GVlut[V])>>8;
        B = (Ylut[Y] + BUlut[U])>>8;

        // Clamping the values here is faster than calling a function
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        outputBuffer[k] = (uint8)R;
        k++;
        outputBuffer[k] = (uint8)G;
        k++;
        outputBuffer[k] = (uint8)B;
        k++;
        outputBuffer[k] = 0xff;
        k++;
      }

      _currentPixels = outputBuffer;
  }
  else
  {
    csReport (_object_reg, CS_REPORTER_SEVERITY_WARNING, QUALIFIED_PLUGIN_NAME,
      "The Theora video stream has an unsupported pixel format.\n");
    return;
  }

  MutexScopedLock lock (_writeMutex);
  _cache.Push (_currentPixels);
}

void csTheoraVideoMedia::SwapBuffers ()
{
  if (_canSwap)
  {
    if (_activeBuffer==0)
    {
      _texture = _buffers[_activeBuffer];
      _activeBuffer = 1;
      _canSwap=false;
    }
    else
    {
      _texture = _buffers[_activeBuffer];
      _activeBuffer = 0;
      _canSwap=false;
    }
  }
}

void csTheoraVideoMedia::WriteData ()
{
  _isWrite=true;

  if (!_canSwap && _cache.GetSize () != 0)
  {
    {
      cachedData pixels = _cache.PopTop ();
      size_t dstSize;
      iTextureHandle* tex = _buffers.Get (_activeBuffer);
      _rgbBuff = tex->QueryBlitBuffer (_streamInfo.pic_x,_streamInfo.pic_y,
                                       _streamInfo.pic_width,_streamInfo.pic_height,dstSize);

      memcpy(_rgbBuff,pixels,_streamInfo.pic_width*_streamInfo.pic_height*4);
      delete pixels;

      tex->ApplyBlitBuffer (_rgbBuff);

      _canSwap=true;
      _isWrite=false;
      _isWriting.NotifyOne ();
    }
  }

  _isWrite=false;
}

void csTheoraVideoMedia::SetCacheSize (size_t size)
{
  _cacheSize = size;
}

bool csTheoraVideoMedia::HasDataReady () const
{
  if (_cache.GetSize ()!=0)
    return true;
  return false;
}
bool csTheoraVideoMedia::IsCacheFull () const
{
  if (_cache.GetSize ()>=_cacheSize)
    return true;
  return false;
}

double csTheoraVideoMedia::GetTargetFPS () const
{
  return _FPS;
}

float csTheoraVideoMedia::GetAspectRatio () const
{
  return _aspectRatio;
}
