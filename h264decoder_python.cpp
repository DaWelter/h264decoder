#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
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

  py::tuple decode_frame_impl(const ubyte *data, int num, int &num_consumed, bool &is_frame_available);
  
public:
  py::tuple decode_frame(const py::str &data_in_str);
  py::list  decode(const py::str &data_in_str);
};


py::tuple PyH264Decoder::decode_frame_impl(const ubyte *data_in, int len, int &num_consumed, bool &is_frame_available)
{
  // see https://docs.python.org/2/c-api/init.html (Releasing the GIL ...)  
  PyThreadState *_save = PyEval_SaveThread();  // allow the python interpreter to run other other threads than the current one.
  
  num_consumed = decoder.parse((ubyte*)data_in, len);
  
  if (is_frame_available = decoder.is_frame_available())
  {
    const auto &frame = decoder.decode_frame();
    int w, h; std::tie(w,h) = width_height(frame);
    
    PyEval_RestoreThread(_save);
    
    // we hold the GIL, so we can safely allocate a python object
    Py_ssize_t out_size = converter.predict_size(w,h);
    py::object py_out_str(py::handle<>(PyString_FromStringAndSize(NULL, out_size)));
    char* out_buffer = PyString_AsString(py_out_str.ptr());
    // allow other python threads again while the conversion runs
    
    _save = PyEval_SaveThread(); 
    const auto &rgbframe = converter.convert(frame, (ubyte*)out_buffer);
    PyEval_RestoreThread(_save);
    
    return py::make_tuple(py_out_str, w, h, row_size(rgbframe));
  }
  else
  {
    PyEval_RestoreThread(_save);
    
    return py::make_tuple(py::object(), 0, 0, 0);
  }
}


py::tuple PyH264Decoder::decode_frame(const py::str &data_in_str)
{
  auto len = PyString_Size(data_in_str.ptr());
  const ubyte* data_in = (const ubyte*)(PyString_AsString(data_in_str.ptr()));

  int num_consumed = 0;
  bool is_frame_available = false;
  auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
  
  return py::make_tuple(frame, num_consumed);
}


py::list PyH264Decoder::decode(const py::str &data_in_str)
{
  int len = PyString_Size(data_in_str.ptr());
  const ubyte* data_in = (const ubyte*)(PyString_AsString(data_in_str.ptr()));
  
  py::list out;
  
  while (len > 0)
  {
    int num_consumed = 0;
    bool is_frame_available = false;
    auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
    
    if (is_frame_available)
    {
      out.append(frame);
    }
    
    len -= num_consumed;
    data_in += num_consumed;
  }
  
  return out;
}


BOOST_PYTHON_MODULE(libh264decoder)
{
  PyEval_InitThreads(); // need for release of the GIL (http://stackoverflow.com/questions/8009613/boost-python-not-supporting-parallelism)
  py::class_<PyH264Decoder>("H264Decoder")
                            .def("decode_frame", &PyH264Decoder::decode_frame)
                            .def("decode", &PyH264Decoder::decode);

  disable_logging();
}