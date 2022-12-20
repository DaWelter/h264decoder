import numpy as np
from PIL import Image
import subprocess
import tempfile
import os
from typing import Tuple

'''
Functions to return ground truth frames and to write them as h264 to disk
'''


def create_frames() -> Tuple[np.ndarray,str]:
    '''
        Makes a blob which moves in a zig-zag pattern top to bottom
    '''
    N = 32
    SIGMA = 4./N
    xgrid,ygrid = np.meshgrid(np.linspace(-1., 1., N), np.linspace(-1.,1.,N))
    ts = np.linspace(-0.8, 0.8, N*N)
    # position:
    xs = 0.8*np.cos(ts*np.pi*N*0.5)
    ys = ts
    # gaussian blob
    vals = np.exp((-(xgrid[None,...]-xs[:,None,None])**2 - (ygrid[None,...]-ys[:,None,None])**2)/(SIGMA**2))
    vals /= np.amax(vals.reshape(-1,N*N),axis=-1)[:,None,None]
    vals = vals*0.9 + 0.1
    # Convert to rgb
    vals = np.stack([vals,vals,vals],axis=-1)
    vals = (vals*255).astype('uint8')
    return vals, 'movingdot.h264'


def create_frames_short_movie() -> Tuple[np.ndarray,str]:
    frames, _ = create_frames()
    frames = frames[[1,-1],...]
    return frames, 'twoframes.h264'


def _write_frames_as_h264(frames : np.ndarray, filename):
    '''
        Writes frames to files, then converts to h264 raw stream using ffmpeg.
    '''
    B, H, W, C = frames.shape
    assert C==3

    with tempfile.TemporaryDirectory() as tmpdirname:
        for i,img in enumerate(frames):
            img = Image.fromarray(img, 'RGB')
            img.save(os.path.join(tmpdirname,f'frame{i:03d}.png'))
        subprocess.check_call(['ffmpeg', '-y', '-i', os.path.join(tmpdirname,r'frame%3d.png'), '-c:v', 'libx264', '-f', 'h264', filename])


if __name__ == '__main__':
    # Only used to regenerate the movie files in the repo.
    frames, filename = create_frames()
    _write_frames_as_h264(frames,filename)
    frames, filename = create_frames_short_movie()
    _write_frames_as_h264(frames,filename)