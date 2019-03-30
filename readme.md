H264 Decoder Python Module
==========================

The aim of this project is to provide a simple decoder for video
captured by a Raspberry Pi camera. At the time of this writing I only
need H264 decoding, since a H264 stream is what the RPi software 
delivers. Furthermore flexibility to incorporate the decoder in larger
python programs in various ways is desirable.

The code might also serve as example for libav and boost python usage.


Files
-----
* `h264decoder.hpp`, `h264decoder.cpp` and `h264decoder_python.cpp` contain the module code.

* Other source files are tests and demos.


Requirements
------------
* Python 2 and 3 should both work.
* cmake for building
* libav
* boost python


Notes:
------
* Linux: To build for Python 3, define Python_ADDITIONAL_VERSIONS. In CMake-Gui you have to manually make a cache entry before configuring. Moreover, this script does not find the correct version of boost python. You have to check and set it manually if needed. E.g. on my Ubuntu system I have libboost_python-py35.so and libboost_python-py27.so. To make a long story short, you probably want to use something like
```cmake -DPython_ADDITIONAL_VERSIONS=3.5 -DBoost_PYTHON_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_python-py35.so ...```
as appropriate for your system.

* Raspberry PI: To build for Python 3, define Python_ADDITIONAL_VERSIONS. In CMake-Gui you have to manually make a cache entry before configuring. Moreover, this script does not find the correct version of boost python. You have to check and set it manually if needed. E.g. on my Ubuntu system I have libboost_python-py35.so and libboost_python-py27.so. To make a long story short, you probably want to use something like
```cmake -DPython_ADDITIONAL_VERSIONS=3.5 -DBoost_PYTHON_LIBRARY_RELEASE=/usr/lib/arm-linux-gnueabihf/libboost_python-py35.so ...```
as appropriate for your system.


* Added experimental support for building with MSVC on windows. I managed to build with the libav distribution from the official download "libav-11.3-win64.7z". Boost python 1.67 built from sources, after applying the patch for some issue (https://github.com/boostorg/python/issues/193). Link to the multi threaded release dll configuration, e.g. boost_python37-vc140-mt-x64-1_67.lib. 
* Building on Linux for 2.7 should be straight forward, provided the requirements are in the usual system locations.
* Routines work with the ```str``` type in Python 2 and ```bytes``` type in Python 3.

Todo
----

* Add a video clip for testing and remove hard coded file names in demos/tests.
* Find boost python for desired python version, at least for boost > 1.67 where it should be possible according to the docs (https://cmake.org/cmake/help/latest/module/FindBoost.html)


License
-------
The code is published under the Mozilla Public License v. 2.0. 
