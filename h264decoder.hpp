#pragma once
/* This file provides a super thin wrapper around functions to decode h264 videos
 * based on libav. This is to keep various heap allocated structures in a common
 * place using RAII for cleanup. And secondly, I prefer to not keep libav things
 * separate from python things.
 * Note that most functions/members throw. This way, error states are conveniently
 * forwarded to python via the exception translation mechanisms of boost::python.  
*/
#include <cstdlib> // for ssize_t (signed int type as large as pointer type)

struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParserContext;
struct SwsContext;
struct AVPacket;

class H264Decoder
{
  /* Persistent things here */
  AVCodecContext        *context;
  AVFrame               *frame;
  AVCodec               *codec;
  AVCodecParserContext  *parser;
  /* In the documentation example on the github master branch, the packet is put on the heap.
     This is done here to store the pointers to the encoded data, which must be kept around 
     between calls to parse- and decode frame.
     In release 11 it is put on the stack. 
  */
  AVPacket              *pkt;
public:
  H264Decoder();
  ~H264Decoder();
  /* First, parse a continuous data stream, dividing it into packets. When there is enough
     data to form a new frame, decode the data and return the frame. 
     
     parse returns the number of consumed bytes of the input stream. It stops consuming bytes
     at frame boundaries.
  */
  ssize_t parse(const unsigned char* in_data, ssize_t in_size);
  bool is_frame_available() const;
  const AVFrame& decode_frame();
};

// TODO: Rename to OutputStage or so?!
class ConverterRGB24
{
  SwsContext *context;
  AVFrame *framergb;
  
public:
  ConverterRGB24();
  ~ConverterRGB24();
   
  /*  Returns, given a width and height, how many bytes the frame buffer is going to need. */
  int predict_size(int w, int h);
  /*  Given a decoded frame, convert it to RGB format and fill out_rgb with the result.
   *  Returns a AVFrame structure holding additional information about the RGB frame,
   *  such as the number of bytes in a row and so on. */
  const AVFrame& convert(const AVFrame &frame, unsigned char* out_rgb);
};

void disable_logging();

/* Wrappers, so we don't have to include libav headers. */
std::pair<int, int> width_height(const AVFrame&);
int row_size(const AVFrame&);

/* all the documentation links
 * My version of libav on ubuntu 16 appears to be from the release/11 branch on github
 * Video decoding example: https://libav.org/documentation/doxygen/release/11/avcodec_8c_source.html#l00455
 * 
 * https://libav.org/documentation/doxygen/release/9/group__lavc__decoding.html
 * https://libav.org/documentation/doxygen/release/11/group__lavc__parsing.html
 * https://libav.org/documentation/doxygen/release/9/swscale_8h.html
 * https://libav.org/documentation/doxygen/release/9/group__lavu.html
 * https://libav.org/documentation/doxygen/release/9/group__lavc__picture.html
 * http://dranger.com/ffmpeg/tutorial01.html
 */