import numpy as np
from PIL import Image
from pathlib import Path
import inspect
import itertools
import threading
import queue
import io

import h264decoder
import movies

def test_module_definitions():
    assert hasattr(h264decoder, "H264Decoder")
    assert hasattr(h264decoder, "Frame")
    assert inspect.getdoc(h264decoder.Frame) != ''


def frame_to_numpy(frame : h264decoder.Frame):
    ''' 
    Converts the frame to a numpy array. Assumes RGB colors, or 3 channels per pixels to be precise.
    '''
    # The frame might have a padding area on the right side. This function removes it.
    decoded = np.frombuffer(frame.data, dtype=np.ubyte)
    decoded = decoded.reshape((frame.height, frame.rowsize//3, 3))
    decoded = decoded[:,:frame.width,:]
    return decoded


def _compare_frames_eq(expected_frames, decoded_frames):
    assert len(expected_frames) == len(decoded_frames), f"Number of decoded frames: {len(decoded_frames)} vs expected {len(expected_frames)}"

    for i, expected, decoded in zip(itertools.count(0), expected_frames, decoded_frames):
        
        expected = np.asarray(expected)

        decoded = frame_to_numpy(decoded)

        # Note: Conversion to int32 to prevent wrap-around
        # Checking for differences
        decoded, expected = np.int32(decoded), np.int32(expected)

        #Image.fromarray(np.uint8(decoded-expected)+128).show()

        assert np.allclose(decoded, expected, atol = 40), f"Image max diff: {np.amax(np.abs(decoded-expected))} at frame {i}"
        mse = np.mean(np.square(decoded-expected))
        assert mse < 10, f"MSE too large: {mse}"


def _feed_decoder(decoder, buffer, max_feed_length, do_flush):
    if max_feed_length is None:
        decoded_frames = decoder.decode(buffer)
    else:
        buffers = np.array_split(np.frombuffer(buffer,dtype=np.uint8), len(buffer)//max_feed_length)
        decoded_frames = sum((decoder.decode(buffer.tobytes()) for buffer in buffers), [])
    if do_flush:
        decoded_frames += decoder.flush()
    return decoded_frames


def _decode_movie(filename, allow_retry=False, max_feed_length=None, do_flush=True):
    dir = Path(__file__).parent

    with open(dir / filename,'rb') as f:
        content = f.read()

    decoder = h264decoder.H264Decoder()
    decoded_frames = _feed_decoder(decoder, content, max_feed_length=max_feed_length, do_flush=do_flush)

    # The decoder seems to figure the input format out too late.
    # By the time our two-frames are passed through it has been unable
    # to output anything.

    # However, it stores some internal info. So that on the second attempt it should work.
    # Probably some initialization is done incorrectly. 
    # Or maybe metadata is missing from the raw h264 stream?
    if not decoded_frames and allow_retry:
        decoded_frames = _feed_decoder(decoder, content, max_feed_length=max_feed_length, do_flush=do_flush)
    
    return decoded_frames


def _decode_mangled_movie(filename):
    dir = Path(__file__).parent

    with open(dir / filename,'rb') as f:
        content = f.read()
    # Inject garbage
    rng = np.random.RandomState(seed=1234567890)
    l = len(content)
    start_corrupt = l//2
    content = np.concatenate([np.frombuffer(content[:start_corrupt],dtype=np.uint8), np.uint8(rng.randint(0,256,size=666)), np.frombuffer(content[start_corrupt:],dtype=np.uint8)]).tobytes()

    decoder = h264decoder.H264Decoder()
    decoded_frames = _feed_decoder(decoder, content, max_feed_length=42, do_flush=False)
    return decoded_frames


def test_short_movie():
    expected_frames, filename = movies.create_frames_short_movie()
    decoded_frames = _decode_movie(filename, allow_retry=True)
    _compare_frames_eq(expected_frames, decoded_frames)

    decoded_frames = _decode_movie(filename, allow_retry=True, max_feed_length=3)
    _compare_frames_eq(expected_frames, decoded_frames)


def test_streaming_like():
    LATENCY_FRAMES=5
    # Latency means frames potentially required to feed before getting anything out.
    # It turns out we still loose frames at the end of the stream even if flush is used, which is however not tested here.
    # Don't flush, thus simulating a streaming app.
    expected_frames, filename = movies.create_frames()
    decoded_frames = _decode_movie(filename, allow_retry=False, do_flush=False)
    assert len(decoded_frames) >= len(expected_frames)-LATENCY_FRAMES, f"Number of decoded frames: {len(decoded_frames)} vs expected {len(expected_frames)}"
    expected_frames = expected_frames[:len(decoded_frames)]
    _compare_frames_eq(expected_frames, decoded_frames)

    # Feed only a few bytes at a time
    decoded_frames = _decode_movie(filename, allow_retry=False, do_flush=False, max_feed_length=42)
    assert len(expected_frames)-LATENCY_FRAMES <= len(decoded_frames) <= len(expected_frames), f"Number of decoded frames: {len(decoded_frames)} vs expected {len(expected_frames)}"
    _compare_frames_eq(expected_frames[:len(decoded_frames)], decoded_frames)

    # Decode a corrupted stream (garbage in the middle of the file)
    decoded_frames = _decode_mangled_movie(filename)
    # Cannot compare frames because we don't know exactly which ones are retrieved because 
    # some are lost in the middle and some are lost at the end. But most should be decoded
    MAX_FRAMES_LOST_TO_CORRUPTION=5
    assert len(expected_frames)-MAX_FRAMES_LOST_TO_CORRUPTION-LATENCY_FRAMES <= len(decoded_frames) <= len(expected_frames)


def test_multithreading():
    '''
    Running two decoders in parallel.
    - Check decoded frames.
    - Check that decoders yielded frames in interleaved order showing concurrent execution.
    It doesn't really ensure that the c++ code executed in parallel but I guess it's good enough.
    '''

    expected_frames, filename = movies.create_frames()
    dir = Path(__file__).parent
    with open(dir / filename,'rb') as f:
        streamdata = f.read()

    q = queue.Queue(maxsize=len(expected_frames)*2)

    def decode_func(worker_id):
        buffer = io.BytesIO(streamdata)
        decoder = h264decoder.H264Decoder()
        max_feed_size = 128
        while True:
            data_in = buffer.read(max_feed_size)
            if not data_in:
                break
            framelist = decoder.decode(data_in)
            q.put_nowait((worker_id, framelist))

    t1 = threading.Thread(target=decode_func, args=(1,))
    t2 = threading.Thread(target=decode_func, args=(2,))

    t1.start()
    t2.start()
    t1.join()
    t2.join()

    def extract(q : queue.Queue):
        while not q.empty():
            yield q.get_nowait()

    items = [ * extract(q) ]

    only_the_worker_ids = [ w for w,x in items ]
    
    assert [*sorted(only_the_worker_ids)] != only_the_worker_ids, "Some degree of parallelism is expected. But it looks like one thread ran after the other."

    frames1 = sum((x for w,x in items if w == 1), [])
    frames2 = sum((x for w,x in items if w == 2), [])

    LATENCY_FRAMES=5
    assert len(expected_frames)-LATENCY_FRAMES <= len(frames1) <= len(expected_frames), f"Number of decoded frames: {len(frames1)} vs expected {len(expected_frames)}"
    assert len(expected_frames)-LATENCY_FRAMES <= len(frames2) <= len(expected_frames), f"Number of decoded frames: {len(frames2)} vs expected {len(expected_frames)}"

    _compare_frames_eq(expected_frames[:len(frames1)], frames1)
    _compare_frames_eq(expected_frames[:len(frames2)], frames2)



if __name__ == '__main__':
    test_module_definitions()
    test_short_movie()
    test_streaming_like()
    test_multithreading()
