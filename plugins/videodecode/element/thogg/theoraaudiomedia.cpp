/*
Copyright (C) 2011 by Alin Baciu

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "isndsys/ss_renderer.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "cstool/initapp.h"

#include "theoraaudiomedia.h"

//SCF_IMPLEMENT_FACTORY (csTheoraAudioMedia)

CS_LEAKGUARD_IMPLEMENT (csTheoraAudioMedia);

csTheoraAudioMedia::csTheoraAudioMedia ()
: scfImplementationType (this), _object_reg (0), _cacheSize (1)
{
}

csTheoraAudioMedia::~csTheoraAudioMedia ()
{
  delete _name;
}

void csTheoraAudioMedia::CleanMedia ()
{
  ogg_stream_clear (&_streamState);

  if (_decodersStarted)
  {
    vorbis_block_clear (&_vorbisBlock);
    vorbis_dsp_clear (&_dspState);
  }
  vorbis_comment_clear (&_streamComments);
  vorbis_info_clear (&_streamInfo);
  fclose (_log);
}

bool csTheoraAudioMedia::Initialize (iObjectRegistry* r)
{
  _object_reg = r;

  // Initialize the decoders
  if (_vorbis_p)
  {
    vorbis_synthesis_init (&_dspState,&_streamInfo);
    vorbis_block_init (&_dspState,&_vorbisBlock);

    _decodersStarted = true;
/*
    csSndSysSoundFormat format;
    format.Bits = _streamInfo.bitrate_nominal;
    format.Channels = _streamInfo.channels;
    format.Freq = _streamInfo.rate;
*/
    if (!csInitializer::RequestPlugins (_object_reg,
      CS_REQUEST_VFS,
      CS_REQUEST_PLUGIN ("crystalspace.sndsys.renderer.software", iSndSysRenderer),
      CS_REQUEST_END))
    {
      csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR,
        "crystalspace.application.vidplaydemo",
        "Can't initialize plugins!");
      return false;
    }

    csRef<iSndSysRenderer> sndrenderer = csQueryRegistry<iSndSysRenderer> (_object_reg);
    if (!sndrenderer) csReport (_object_reg, CS_REPORTER_SEVERITY_ERROR,
                                "crystalspace.application.vidplaydemo",
                                "Failed to locate sound renderer!");

    _log = fopen ("sndlog.txt","wb");
  }
  else
  {
    // Tear down the partial Vorbis setup
    vorbis_info_clear (&_streamInfo);
    vorbis_comment_clear (&_streamComments);
    _decodersStarted = false;
  }
  return 0;
}

const char* csTheoraAudioMedia::GetName () const
{
  return _name;
}

const char* csTheoraAudioMedia::GetType () const
{
  return "TheoraAudio";
}

unsigned long csTheoraAudioMedia::GetFrameCount () const
{
  return 0;
}

float csTheoraAudioMedia::GetDuration () const
{
  return _length;
}

double csTheoraAudioMedia::GetPosition () const
{
  return _streamState.granulepos;
}

bool csTheoraAudioMedia::Update ()
{
  if (_cache.GetSize ()>=_cacheSize)
    return false;

  _audiobuf_ready=false;

  while (_vorbis_p && !_audiobuf_ready)
  {
    float **pcm;
    int ret=vorbis_synthesis_pcmout (&_dspState,&pcm);
    int count = 0;

    // ToDo: change 714 to the frame count of the video !!!!!!!!
    int numSamples = 714 * _streamInfo.channels;
    int numBytes = numSamples * sizeof (short);

    short *samples = new short[numBytes];

    // If there's pending, decoded audio, grab it
    if (ret>0)
    {
      int i,j;

      for (i=0;i<ret && i< (2048/_streamInfo.channels);i++)
        for (j=0;j<_streamInfo.channels;j++)
        {
          int val= (int) (pcm[j][i]*32767.f);
          if (val>32767)
            val=32767;
          if (val<-32768)
            val=-32768;
          samples[count]=val;
          count++;
        }
        _audiobuf_ready=1;
        vorbis_synthesis_read (&_dspState,i);

        cachedData dataOut;

        dataOut.count=count;
        dataOut.data=samples;

        _cache.Push (dataOut);

        samples = nullptr;
    }
    else
    {
      // No pending audio; is there a pending packet to decode?
      if (ogg_stream_packetout (&_streamState,&_oggPacket)>0)
      {
        if (vorbis_synthesis (&_vorbisBlock,&_oggPacket)==0)  // test for success!
          vorbis_synthesis_blockin (&_dspState,&_vorbisBlock);
      }
      else   // We need more data; break out to suck in another page
        break;
    }

  }

  if (_audiobuf_ready)
    return 0;

  return 1;
}

void csTheoraAudioMedia::DropFrame ()
{
  if (_cache.GetSize ()!=0)
    _cache.PopTop ();
}

void csTheoraAudioMedia::SetPosition (float time, ogg_sync_state *oy,ogg_page *op,ogg_stream_state *thState)
{
  ogg_stream_reset (&_streamState);
  vorbis_synthesis_restart (&_dspState);

  // Let's decode some pages and seek to the appropriate PCM sample
  ogg_int64_t granule=0;
  //float last_page_time=time;

  while (true)
  {
    int ret=ogg_sync_pageout ( oy, op );
    if (ret == 1)
    {
      int serno=ogg_page_serialno (op);
      if (serno == _streamState.serialno)
      {
        granule=ogg_page_granulepos (op);
        float g_time= (float)vorbis_granule_time (&_dspState,granule);
        if (g_time > time)
        {
          float **pcm;
          int len = vorbis_synthesis_pcmout (&_dspState,&pcm);
          if (len > 0)
            break;

          //ogg_stream_pagein (&mInfo->VorbisStreamState,&mInfo->OggPage);
          time=g_time;
          break;
        }
        //last_page_time=g_time;
      }
      else ogg_stream_pagein (thState,op);
    }
    else
    {
      char *buffer = ogg_sync_buffer ( oy, 4096);
      int bytesRead = fread (buffer,sizeof (char),4096,_infile);
      if (bytesRead == 0) break;
      ogg_sync_wrote ( oy, bytesRead );
    }
  }

}

void csTheoraAudioMedia::SwapBuffers ()
{
}

void csTheoraAudioMedia::InitializeStream (const char* name,  ogg_stream_state &state,
                                           vorbis_info &info, vorbis_comment &comments,
                                           FILE *source)
{
  _name = new char[strlen (name)];
  strcpy (_name, name);
  memcpy (&_streamState,&state,sizeof (state));
  memcpy (&_streamInfo,&info,sizeof (info));
  memcpy (&_streamComments,&comments,sizeof (comments));
  _vorbis_p=1;
  _infile = source;
  _decodersStarted = false;
}

void csTheoraAudioMedia::WriteData ()
{
  if (_cache.GetSize ()!=0)
  {
    cachedData data = _cache.PopTop ();
    delete data.data;
  }
}

void csTheoraAudioMedia::SetCacheSize (size_t size)
{
  _cacheSize = size;
}

bool csTheoraAudioMedia::HasDataReady () const
{
  if (_cache.GetSize ()!=0)
    return true;
  return false;
}

bool csTheoraAudioMedia::IsCacheFull () const
{
  if (_cache.GetSize ()>=_cacheSize)
    return true;
  return false;
}
