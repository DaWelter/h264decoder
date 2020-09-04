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
* Python 3
* cmake for building
* libav / ffmpeg (swscale, avutil and avcodec)
* pybind11 (will be automatically downloaded from github if not found)

I tested it on

* Ubuntu 18, gcc 9, Anaconda environment with Python 3.7, Libav from Ubuntu repo.
* Windows 10, Visual Studio Community 2017, Anaconda environment with Python 3.7, FFMPEG from vcpkg.

Building and Installing
-----------------------

### Windows

The suggested way to obtain ffmpeg is through [vcpkg](https://github.com/microsoft/vcpkg). Assuming all the setup including VC integration has been done, we can install the x64 libraries with

```cmd
vcpkg.exe install ffmpeg:x64-windows
```

We can build the extension module with setuptools almost normally. However cmake is used internally and we have to let it know the search paths to our libs. Hence the additional ```--cmake-args``` argument with the toolchain file as per vcpkg instructions.

```bash
python setup.py build_ext --cmake-args="-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake"
pip install -e .
```

The ```-e``` option installs symlinks to the build directory. Useful for development. Leave it out otherwise.

----------------------------------------------

Alternatively one can build the extension module manually with cmake.
From the project directory:
```cmd
mkdir [build dir name]
cd [build dir name]
cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake -A x64 ..
cmake --build .
```

### Linux

Should be a matter of installing the libav or ffmpeg libraries. On Debian or Ubuntu:

```bash
sudo apt install libswscale-dev libavcodec-dev libavutil-dev
```

And then running

```bash
pip install .
```

in the project directory.


Credits
-------

* [Michael Welter](https://github.com/DaWelter). Original author.
* [Martin Valgur](https://github.com/valgur).  Switch to pybind11, nice build configuration and more.

License
-------
The code is published under the Mozilla Public License v. 2.0. 
