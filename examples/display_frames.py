#!/usr/bin/env python3

import numpy as np
import h264decoder
import matplotlib.pyplot as pyplot
import sys

if len(sys.argv)<2:
  print ("Usage: {} video".format(sys.argv[0]))
  sys.exit(-1)

thefile = sys.argv[1]

img = None
fig, ax = pyplot.subplots(1,1)

def display(framedata):
  global img, fig, ax
  (frame, w, h, ls) = framedata
  if frame is not None:
    print('frame size %i bytes, w %i, h %i, linesize %i' % (len(frame), w, h, ls))
    frame = np.frombuffer(frame, dtype=np.ubyte, count=len(frame))
    frame = frame.reshape((h, ls//3, 3))
    frame = frame[:,:w,:]
    
    if not img:
      img = ax.imshow(frame)
      pyplot.show(block = False)
    else:
      img.set_data(frame)
      pyplot.draw()
    pyplot.pause(0.001)

# Two APIs to decode frames
def run_decode_frame(decoder, data_in):
  while len(data_in):
    # One frame at a time, indicating how much of the input has been consumed.
    framedata, nread = decoder.decode_frame(data_in)
    data_in = data_in[nread:]
    display(framedata)  

def run_decode(decoder, data_in):
  # Consume the input completely. May result in multiple frames read.
  # This runs a bit faster than the other way.
  framedatas = decoder.decode(data_in)
  for framedata in framedatas:
    display(framedata)

f = open(thefile, 'rb')
decoder = h264decoder.H264Decoder()
while 1:
  data_in = f.read(1024)
  if not data_in:
    break
  run_decode(decoder, data_in)