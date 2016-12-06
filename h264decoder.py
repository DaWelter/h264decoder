#!/usr/bin/env python2

import os
import sys
import numpy as np

import libh264decoder

import matplotlib.pyplot as pyplot

decoder = libh264decoder.H264Decoder()

fig, ax = pyplot.subplots(1,1)
img = None

f = open('testclip.h264','r')
while 1:
  data_in = f.read(1024)
  frame, w, h, ls = decoder.decode(data_in)
  
  if frame is not None:
    print 'frame size %i bytes, w %i, h %i, linesize %i' % (len(frame), w, h, ls)
    frame = np.fromstring(frame, dtype = np.ubyte, count = len(frame), sep = '')
    frame = frame.reshape((h, ls/3, 3))
    frame = frame[:,:w,:]
    
    if not img:
      img = ax.imshow(frame)
      pyplot.show(block = False)
    else:
      img.set_data(frame)
      pyplot.draw()
    pyplot.pause(0.001)
  
  if not data_in:
    break
