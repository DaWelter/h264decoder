#!/usr/bin/env python3
from __future__ import print_function
# get samples from http://www.fastvdo.com/H.264.html
#                  http://www.h264info.com/?page_id=20 aka http://www.h264info.com/clips.html

import os
import sys
import numpy as np

import libh264decoder

import matplotlib.pyplot as pyplot


img = None
fig, ax = pyplot.subplots(1,1)

def display(framedata):
  global img, fig, ax
  (frame, w, h, ls) = framedata
  if frame is not None:
    print('frame size %i bytes, w %i, h %i, linesize %i' % (len(frame), w, h, ls))
    frame = np.frombuffer(frame, dtype = np.ubyte, count = len(frame))
    frame = frame.reshape((h, ls//3, 3))
    frame = frame[:,:w,:]
    
    if not img:
      img = ax.imshow(frame)
      pyplot.show(block = False)
    else:
      img.set_data(frame)
      pyplot.draw()
    pyplot.pause(0.0001)

def run_decode_frame(decoder, data_in):
  while len(data_in):
    framedata, nread = decoder.decode_frame(data_in)
    data_in = data_in[nread:]
    display(framedata)  

def run_decode(decoder, data_in):
  framedatas = decoder.decode(data_in)
  for framedata in framedatas:
    display(framedata)

decoder = libh264decoder.H264Decoder()
with open('FVDO_Plane_4cif.264','rb') as f:
  while 1:
    data_in = f.read(1024)
    print((data_in))
   # print(data_in)
    if not data_in:
      break
    run_decode(decoder, data_in)