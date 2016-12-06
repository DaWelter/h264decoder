#!/usr/bin/env python2

import os
import sys
import numpy as np
import time

import libh264decoder

decoder = libh264decoder.H264Decoder()

t0 = time.time()
num_frames = 0

f = open('testclip.h264','r')
while 1:
  data_in = f.read(1024)
  frame, w, h, ls = decoder.decode(data_in)
  
  if frame is not None:
    frame = np.fromstring(frame, dtype = np.ubyte, count = len(frame), sep = '') # this conversion drops fps from 200 to 150
    frame = frame.reshape((h, ls/3, 3))
    frame = frame[:,:w,:]
    num_frames += 1
  
  if not data_in:
    break
print '\n',
t1 = time.time()
print 'fps = ', (num_frames/(t1-t0))
