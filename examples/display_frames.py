#!/usr/bin/env python3

import numpy as np
import h264decoder
import matplotlib.pyplot as pyplot
import sys
import argparse


def frame_to_numpy(frame : h264decoder.Frame):
    ''' 
    Converts the frame to a numpy array. Assumes RGB colors, or 3 channels per pixels to be precise.
    '''
    # The frame might have a padding area on the right side. This function removes it.
    decoded = np.frombuffer(frame.data, dtype=np.ubyte)
    decoded = decoded.reshape((frame.height, frame.rowsize//3, 3))
    decoded = decoded[:,:frame.width,:]
    return decoded


def display(ui, frame : h264decoder.Frame):
  if ui is None:
    img = None
    fig, ax = pyplot.subplots(1,1)
  else:
    img, fig, ax = ui
  if frame is not None:
    #print('frame size %i bytes, w %i, h %i, linesize %i' % (len(frame), w, h, ls))
    frame = frame_to_numpy(frame)  
    if not img:
      img = ax.imshow(frame)
      pyplot.show(block = False)
    else:
      img.set_data(frame)
      pyplot.draw()
    pyplot.pause(0.001)
  return (img, fig, ax)


def display_h264_file(filename):
  decoder = h264decoder.H264Decoder()
  ui = None
  with open(filename, 'rb') as f:
    while (data_in := f.read(1024)):
      # Consume the input completely. May result in multiple frames read.
      framedatas = decoder.decode(data_in)
      for framedata in framedatas:
        ui = display(ui, framedata)


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Displays raw h264 files")
  parser.add_argument("file", type=str, help="Input filename")
  args = parser.parse_args()
  display_h264_file(args.file)