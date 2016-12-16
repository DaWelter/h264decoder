#!/usr/bin/env python2

import os
import sys
import numpy as np
import time

import libh264decoder

decoder = libh264decoder.H264Decoder()

t0 = time.time()
num_frames = 0

if 1:
  def conv(frame):
    frame = np.fromstring(frame, dtype = np.ubyte, count = len(frame), sep = '') # this conversion drops fps from 200 to 150
    frame = frame.reshape((h, ls/3, 3))
    frame = frame[:,:w,:]
else:
  def conv(frame):
    pass

f = open('testclip.h264','r')
# Original way is 60 fps on laptop, this way is 100 fps
while 1:
  data_in = f.read(1024)
  if not data_in:
    break
  framelist = decoder.decode(data_in)
  for frame, w, h, ls in framelist:
    conv(frame)
    num_frames += 1
# On laptop this way is 80 fps.
while 0:
  data_in = f.read(1024)
  if not data_in:
    break
  while data_in:
    (frame, w, h, ls), nread = decoder.decode_frame(data_in)
    data_in = data_in[nread:]
    if frame:
      conv(frame)
      num_frames += 1
print '\n',
t1 = time.time()
print 'fps = ', (num_frames/(t1-t0))
