/* I'm such a bad script kiddie. This code is entirely based on roxlu's code http://roxlu.com/2014/039/decoding-h264-and-yuv420p-playback
*/


#include <vector>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
}

#include "h264decoder.hpp"

typedef unsigned char ubyte;
typedef unsigned long ulong;


H264Decoder::H264Decoder()
{
  avcodec_register_all();

  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!codec)
    throw std::runtime_error("cannot find decoder");
  
  context = avcodec_alloc_context3(codec);
  if (!context)
    throw std::runtime_error("cannot allocate context");

  if(codec->capabilities & CODEC_CAP_TRUNCATED) {
    context->flags |= CODEC_FLAG_TRUNCATED;
  }  

  int err = avcodec_open2(context, codec, nullptr);
  if (err < 0)
    throw std::runtime_error("cannot open context");

  parser = av_parser_init(AV_CODEC_ID_H264);
  if (!parser)
    throw std::runtime_error("cannot init parser");
  
  frame = av_frame_alloc();
  if (!frame)
    throw std::runtime_error("cannot allocate frame");
}


H264Decoder::~H264Decoder()
{
  av_parser_close(parser);
  avcodec_close(context);
  av_free(context);
  av_frame_free(&frame);
}


const AVFrame* H264Decoder::next(const ubyte* in_data, ulong in_size)
{ 
  AVPacket pkt_mem;
  auto pkt = &pkt_mem; // might want to use heap allocated pointer later on ...
  av_init_packet(pkt);  
 
  buffer.insert(buffer.end(), in_data, in_data + in_size);
  
  int nread = av_parser_parse2(parser, context, &pkt->data, &pkt->size, 
                               buffer.size() ? &buffer[0] : nullptr, buffer.size(), 
                               0, 0, AV_NOPTS_VALUE);

  //printf("inserted %ld bytes in buffer of subsequent size %ld, of which %i bytes were consumed\n", in_size, buffer.size(), nread);
  
  // I'm guestimating that 4 out of 5 times (nread == buffer.size())
  // There may be some optimization opportunity here.
  buffer.erase(buffer.begin(), buffer.begin() + nread);
  
  // size and buffer refer to a buffer with data of a new frame. But only if all data for that frame is present. Otherwise these vars are zeroed out. 
  if (pkt->size && pkt->data)
  {
    int got_picture = 0;
    
    nread = avcodec_decode_video2(context, frame, &got_picture, pkt);
    if (nread < 0)
      throw std::runtime_error("error decoding frame\n");
    
    if (got_picture)
    {
      return frame;
    }
  }
  
  return nullptr;
}




ConverterRGB24::ConverterRGB24()
{
  framergb = av_frame_alloc();
  if (!framergb)
    throw std::runtime_error("cannot allocate frame");
  context = nullptr;
}

ConverterRGB24::~ConverterRGB24()
{
  sws_freeContext(context);
  av_frame_free(&framergb);
}


const AVFrame* ConverterRGB24::convert(const AVFrame *frame, ubyte* out_rgb)
{
  int w = frame->width;
  int h = frame->height;
  int pix_fmt = frame->format;
  
  context = sws_getCachedContext(context, 
                                 w, h, (AVPixelFormat)pix_fmt, 
                                 w, h, PIX_FMT_RGB24, SWS_BILINEAR, 
                                 nullptr, nullptr, nullptr);
  if (!context)
    throw std::runtime_error("cannot allocate context");
  
  // Setup framergb with out_rgb as external buffer. Also say that we want RGB24 output.
  avpicture_fill((AVPicture*)framergb, out_rgb, PIX_FMT_RGB24, w, h);
  // Do the conversion.
  sws_scale(context, frame->data, frame->linesize, 0, h,
            framergb->data, framergb->linesize);
  framergb->width = w;
  framergb->height = h;
  return framergb;
}

/*
 Returns, given a width and height, how many bytes the frame buffer is going to need.

 * WARNING:
 * avpicture_get_size is used in http://dranger.com/ffmpeg/tutorial01.html 
 * to determine the size of the output frame buffer. However, avpicture_get_size returns
 * the size of a compact representation, without padding bytes. On the other hand,
 * avpicture_fill will require a larger buffer when linesize > width.
 */
int ConverterRGB24::predict_size(int w, int h)
{
  return avpicture_fill((AVPicture*)framergb, nullptr, PIX_FMT_RGB24, w, h);  
}



void disable_logging()
{
  av_log_set_level(AV_LOG_QUIET);
}

