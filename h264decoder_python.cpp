#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <Python.h>
}


#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
namespace py = boost::python;
// python string api
// https://docs.python.org/2/c-api/string.html

#include "h264decoder.hpp"

typedef unsigned char ubyte;



class PyH264Decoder
{
  H264Decoder decoder;
  ConverterRGB24 converter;
  
public:
  py::object decode(const py::str &data_in_str);
};


py::object PyH264Decoder::decode(const py::str &data_in_str)
{
  Py_ssize_t len = PyString_Size(data_in_str.ptr());
  char* data_in = PyString_AsString(data_in_str.ptr());
  const AVFrame* frame = nullptr;
  const AVFrame* rgbframe = nullptr;
  //fprintf(stderr, "PyH264Decoder::decode: %ld bytes in\n", len);
  
  // see https://docs.python.org/2/c-api/init.html (Releasing the GIL ...)
  Py_BEGIN_ALLOW_THREADS  // allow the python interpreter to run other other threads than the current one.
  frame = decoder.next((ubyte*)data_in, len);
  Py_END_ALLOW_THREADS
  
  if (frame)  // new frame decoded?
  {
    // we hold the GIL, so we can safely allocate a python object
    Py_ssize_t out_size = converter.predict_size(frame->width, frame->height);
    py::object py_out_str(py::handle<>(PyString_FromStringAndSize(NULL, out_size)));
    char* out_buffer = PyString_AsString(py_out_str.ptr());
  
    Py_BEGIN_ALLOW_THREADS // allow other python threads again while the conversion runs
    rgbframe = converter.convert(frame, (ubyte*)out_buffer);
    Py_END_ALLOW_THREADS
  
    //fprintf(stderr, "PyH264Decoder::decode: %ld bytes out, w = %i, h = %i, ls = %i\n", out_size, rgbframe->width, rgbframe->height, rgbframe->linesize[0]);
    return py::make_tuple(py_out_str, py::object(rgbframe->width), py::object(rgbframe->height), py::object(rgbframe->linesize[0]));
  }
  else
    return py::make_tuple(py::object(), 0, 0, 0);
}


// #ifdef DEBUG
// BOOST_PYTHON_MODULE(libh264decoder)
// #else
BOOST_PYTHON_MODULE(libh264decoder)
// #endif
{
  PyEval_InitThreads(); // need for release of the GIL (http://stackoverflow.com/questions/8009613/boost-python-not-supporting-parallelism)
  py::class_<PyH264Decoder>("H264Decoder")
                            .def("decode", &PyH264Decoder::decode);

  disable_logging();
}