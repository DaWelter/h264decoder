#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
  #include <libavcodec/avcodec.h>
}

#include "h264decoder.hpp"

typedef unsigned char ubyte;


/* shamelessly copy pasted from
 * http://dranger.com/ffmpeg/tutorial01.html
*/
void SaveFrame(const AVFrame *pFrame, int iFrame) 
{
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%02d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", pFrame->width, pFrame->height);
  
  // Write pixel data
  for(y=0; y<pFrame->height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, pFrame->width*3, pFile);
  
  // Close file
  fclose(pFile);
}



int main(int argc, char *argv[])
{
  H264Decoder decoder;
  ConverterRGB24 converter;
  disable_logging();
  
  std::FILE* fp = std::fopen(argv[1], "r");
  if (!fp)
  {
    fprintf(stderr, "cannot open file %s\n", argv[1]);
    return -1;
  }
  
  int frame_num = 0;
  std::vector<ubyte> framebuffer;
  std::vector<ubyte> buffer(1024);
  for (;;)
  {
    int n = std::fread(&buffer[0], 1, buffer.size(), fp);
    if (n <= 0) break;
    
    const AVFrame* frame = decoder.next(&buffer[0], n);
    if (frame)
    {
      //NOTE: pixel format of enum symbol named AV_PIX_FMT_YUV420P comes out of h264 decoder. 
      // This symbols happens to have numeric value zero. So it is no bug when frame->format == 0.
      printf("frame decoded: %i x %i, fmt = %i\n", frame->width, frame->height, frame->format);
      
      framebuffer.resize(converter.predict_size(frame->width, frame->height));
      const AVFrame* rgbframe = converter.convert(frame, &framebuffer[0]);
      
      ++frame_num;
      if (frame_num < 10)
        SaveFrame(rgbframe, frame_num);
    }
  }
  
  return 0;
}
