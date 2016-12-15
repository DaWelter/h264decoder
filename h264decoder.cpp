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


#if (LIBAVCODEC_VERSION_MAJOR <= 54) 
#  define av_frame_alloc avcodec_alloc_frame
#  define av_frame_free  avcodec_free_frame  
#endif


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

#if 1
  pkt = new AVPacket;
  if (!pkt)
    throw std::runtime_error("cannot allocate packet");
  av_init_packet(pkt);
#endif
}


H264Decoder::~H264Decoder()
{
  av_parser_close(parser);
  avcodec_close(context);
  av_free(context);
  av_frame_free(&frame);
#if 1
  delete pkt;
#endif
}

#if 0
const AVFrame* H264Decoder::next(const ubyte* in_data, ulong in_size)
{ 
  buffer.insert(buffer.end(), in_data, in_data + in_size);
  
  /* Points pkt->data to input buffer if input buffer contains a full frame. 
     Otherwise a full frame is accumulated in a extra buffer behind the scenes. Then pkt->data is pointed to this. 
     And the owner of the memory of pkt->data appears to be either the user of the lib or the "context" depending on which of the above cases we have.
     Therefore, in order to store a packet for later, one would have to make a copy of pkt with appropriate API functions - with possible exception of manual copy of the buffer memory. 
  */
  int nread = av_parser_parse2(parser, context, &pkt->data, &pkt->size, 
                               buffer.size() ? &buffer[0] : nullptr, buffer.size(), 
                               0, 0, AV_NOPTS_VALUE);

  printf("inserted %ld bytes in buffer of subsequent size %ld, of which %i bytes were consumed\n", in_size, buffer.size(), nread);
  printf("  pkt data    = %lx, size = %d\n", (std::size_t)pkt->data, pkt->size);
  printf("  buffer data = %lx\n", (std::size_t)&buffer[0]);
  
  // I'm guestimating that 4 out of 5 times (nread == buffer.size())
  // There may be some optimization opportunity here.
  buffer.erase(buffer.begin(), buffer.begin() + nread);
  
  // size and buffer refer to a buffer with data of a new frame. But only if all data for that frame is present. Otherwise these vars are zeroed out. 
  if (pkt->size)
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
#endif


ulong H264Decoder::parse(const ubyte* in_data, ulong in_size)
{
  auto nread = av_parser_parse2(parser, context, &pkt->data, &pkt->size, 
                                in_data, in_size, 
                                0, 0, AV_NOPTS_VALUE);
  return nread;
}


bool H264Decoder::is_frame_available() const
{
  return pkt->size > 0;
}


const AVFrame& H264Decoder::decode_frame()
{
  int got_picture = 0;
  int nread = avcodec_decode_video2(context, frame, &got_picture, pkt);
  if (nread < 0 || got_picture == 0)
    throw std::runtime_error("error decoding frame\n");
  return *frame;
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


const AVFrame& ConverterRGB24::convert(const AVFrame &frame, ubyte* out_rgb)
{
  int w = frame.width;
  int h = frame.height;
  int pix_fmt = frame.format;
  
  context = sws_getCachedContext(context, 
                                 w, h, (AVPixelFormat)pix_fmt, 
                                 w, h, PIX_FMT_RGB24, SWS_BILINEAR, 
                                 nullptr, nullptr, nullptr);
  if (!context)
    throw std::runtime_error("cannot allocate context");
  
  // Setup framergb with out_rgb as external buffer. Also say that we want RGB24 output.
  avpicture_fill((AVPicture*)framergb, out_rgb, PIX_FMT_RGB24, w, h);
  // Do the conversion.
  sws_scale(context, frame.data, frame.linesize, 0, h,
            framergb->data, framergb->linesize);
  framergb->width = w;
  framergb->height = h;
  return *framergb;
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



std::pair<int, int> width_height(const AVFrame& f)
{
  return std::make_pair(f.width, f.height);
}

int row_size(const AVFrame& f)
{
  return f.linesize[0];
}


void disable_logging()
{
  av_log_set_level(AV_LOG_QUIET);
}

