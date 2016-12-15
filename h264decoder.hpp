#ifndef H264DECODER_HPP
#define H264DECODER_HPP

#include <vector>

struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParserContext;
struct SwsContext;
struct AVPacket;

class H264Decoder
{
  AVCodecContext        *context;
  AVFrame               *frame;
  AVCodec               *codec;
  AVCodecParserContext  *parser;
  // In the documentation example on the github master branch, the packet is put on the heap.
  // In release 11 it is put on the stack, which is what we do here, too ... or we don't, depending on my mood.
  AVPacket              *pkt;
  //std::vector<unsigned char>  buffer;
public:
  H264Decoder();
  ~H264Decoder();
  
  ulong parse(const unsigned char* in_data, ulong in_size);
  bool is_frame_available() const;
  const AVFrame& decode_frame();
};

// TODO: Rename OutputStage?
class ConverterRGB24
{
  SwsContext *context;
  AVFrame *framergb;
  
public:
  ConverterRGB24();
  ~ConverterRGB24();
  
  int predict_size(int w, int h);
  const AVFrame& convert(const AVFrame &frame, unsigned char* out_rgb);
};

void disable_logging();

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

#endif
