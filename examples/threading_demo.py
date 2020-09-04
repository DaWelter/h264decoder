#!/usr/bin/env python3

import threading
import h264decoder
import sys

if len(sys.argv)<2:
  print ("Usage: {} video".format(sys.argv[0]))
  sys.exit(-1)

thefile = sys.argv[1]

class DecoderThread(threading.Thread):
  def __init__(self):
    threading.Thread.__init__(self)
    self.counter = 0
  def run(self):
    with open(thefile, 'rb') as f:
      num_frames = 0
      decoder = h264decoder.H264Decoder()
      # Original way is 60 fps on laptop, this way is 100 fps
      while 1:
        data_in = f.read(1024)
        if not data_in:
          break
        framelist = decoder.decode(data_in)
        for frame in framelist:
          print('thread %s decoded frame %i' % (self.ident, self.counter))
          self.counter += 1
      return num_frames

a = DecoderThread()
b = DecoderThread()
a.start()
b.start()
a.join()
b.join()