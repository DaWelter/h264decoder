H264 Decoder Python Module
==========================

The aim of this project is to provide a simple decoder for video
captured by a Raspberry Pi camera. At the time of this writing I only
need H264 decoding, since a H264 stream is what the RPi software 
delivers. Furthermore flexibility to incorporate the decoder in larger
python programs in various ways is desirable.

The code might also serve as example for libav and pybind11 usage.


Files
-----
* `h264decoder.hpp`, `h264decoder.cpp` and `h264decoder_python.cpp` contain the module code.

* Other source files are tests and demos.


Requirements
------------
* cmake for building
* libav / ffmpeg (swscale, avutil and avcodec)

Building and Installing
-----------------------

### Windows
The suggested way to obtain ffmpeg is through [vcpkg](https://github.com/microsoft/vcpkg). Assuming all the setup including VC integration has been done, we can install the x64 libraries with
```
.\vcpkg.exe install ffmpeg:x64-windows
```
We can build the extension module with setuptools almost normally. However cmake is used internally and we have to let it know the search paths to our libs. Hence the additional ```--cmake-args``` argument with the toolchain file as per vcpkg instructions.
```
python setup.py build_ext --cmake-args="-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake"
pip install -e .
```
The ```-e``` option installs symlinks to the build directory. Useful for development. Leave it out otherwise.

----------------------------------------------

Alternatively one can build the extension module manually with cmake.
From the project directory:
```
mkdir [build dir name]
cd [build dir name]
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake -A x64 ..
cmake --build .
```

### Linux

Should be a matter of installing the libav libraries and then running
```
pip install .
```
in the project directory.


Credits
-------
* [Michael Welter](https://github.com/DaWelter) Original author.
* [Martin Valgur](https://github.com/valgur) Switch to pybind11, nice build configuration and more.

Todo
----

* Add a video clip for testing and remove hard coded file names in demos/tests.


License
-------
The code is published under the Mozilla Public License v. 2.0. 
