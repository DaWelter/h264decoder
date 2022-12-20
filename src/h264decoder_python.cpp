#include <cstdio>
#include <stdexcept>
#include <cassert>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#include <iostream>

#include "h264decoder.hpp"

namespace py = pybind11;

using ubyte = unsigned char;

class GILScopedReverseLock
{
  // see https://docs.python.org/2/c-api/init.html (Releasing the GIL ...)  
public:
  GILScopedReverseLock() 
    : state(nullptr)
  {
    unlock();
  }
  
  ~GILScopedReverseLock()
  {
    lock();
  }
  
  void lock() 
  {
    // Allow successive calls to lock. 
    // E.g. lock() followed by destructor.
    if (state != nullptr)
    {
      PyEval_RestoreThread(state);
      state = nullptr;
    }
  }
  
  void unlock()
  {
    assert (state == nullptr);
    state = PyEval_SaveThread();
  }
  
  GILScopedReverseLock(const GILScopedReverseLock &) = delete;
  GILScopedReverseLock(const GILScopedReverseLock &&) = delete;
  GILScopedReverseLock operator=(const GILScopedReverseLock &) = delete;
  GILScopedReverseLock operator=(const GILScopedReverseLock &&) = delete;
private:
  PyThreadState *state;
};


py::object create_named_tuple_return_type()
{
  py::exec("import typing");
  return py::eval("typing.NamedTuple('Frame',[ ('data' , bytes), ('width' , int), ('height' , int), ('rowsize' , int ) ])");
}


/* The class wrapped in python via boost::python */
class PyH264Decoder
{
  H264Decoder decoder;
  ConverterRGB24 converter;

  py::object create_frame(py::bytes data, int w, int h, int rowsize);
  /* Extract frames from input stream. Stops at frame boundaries and returns the number of consumed bytes
   * in num_consumed.
   * 
   * If a frame is completed, is_frame_available is set to true, and the returned python tuple contains
   * formation about the frame as well as the frame buffer memory. 
   * 
   * Else, i.e. all data in the buffer is consumed, is_frame_available is set to false. The returned tuple
   * contains dummy data.
   */ 
  py::tuple decode_frame_impl(const ubyte *data, ssize_t len, ssize_t &num_consumed, bool &is_frame_available);
public:
  /* Decoding style analogous to c/c++ way. Stop at frame boundaries. 
   * Return tuple containing frame data as above as nested tuple, and an integer telling how many bytes were consumed.  */
  py::tuple decode_frame(const py::bytes &data_in_str);
  /* Process all the input data and return a list of all contained frames. */
  py::list  decode(const py::bytes &data_in_str);

  py::list flush();
};


py::object PyH264Decoder::create_frame(py::bytes data, int w, int h, int rowsize)
{
  static auto frame_type = py::module::import("h264decoder").attr("Frame");
  return frame_type(data, w, h, rowsize);
}


py::tuple PyH264Decoder::decode_frame_impl(const ubyte *data_in, ssize_t len, ssize_t &num_consumed_out, bool &is_frame_available)
{
  GILScopedReverseLock gilguard;
  
  const auto [num_consumed, frame] = decoder.parse(data_in, len);
  num_consumed_out = num_consumed;
  is_frame_available = frame != nullptr;

  if (frame)
  {
    int w, h; std::tie(w,h) = width_height(*frame);
    Py_ssize_t out_size = converter.predict_size(w,h);

    gilguard.lock();

    // Allocate storage for the frame
    // Pybind11 takes ownership over the created buffer.
    py::bytes py_out_str = py::reinterpret_steal<py::bytes>(PyBytes_FromStringAndSize(nullptr, out_size));
    char* out_buffer = PyBytes_AsString(py_out_str.ptr());

    // Copy the final frame into the buffer
    gilguard.unlock();
    const auto &rgbframe = converter.convert(*frame, (ubyte*)out_buffer);
    gilguard.lock();

    return create_frame(py_out_str, w, h, row_size(rgbframe));
  }
  else
  {
    gilguard.lock();
    return create_frame(py::bytes(), 0, 0, 0);
  }
}


py::tuple PyH264Decoder::decode_frame(const py::bytes &data_in_str)
{
  PyErr_WarnEx(PyExc_DeprecationWarning, 
                "decode_frame() is deprecated and will be removed. Use decode() instead! It is easier to use and runs just as fast.", 
                1);

  ssize_t len = PyBytes_Size(data_in_str.ptr());
  auto data_in = reinterpret_cast<const ubyte*>(PyBytes_AsString(data_in_str.ptr()));

  ssize_t num_consumed = 0;
  bool is_frame_available = false;
  auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
  
  return py::make_tuple(frame, num_consumed);
}


py::list PyH264Decoder::decode(const py::bytes &data_in_str)
{
  ssize_t len = PyBytes_Size(data_in_str.ptr());
  auto data_in = reinterpret_cast<const ubyte*>(PyBytes_AsString(data_in_str.ptr()));

  py::list out;
  
  bool was_data_consumed = true;

  while (len > 0)
  {
    ssize_t num_consumed = 0;
    bool is_frame_available = false;

    auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
    if (is_frame_available)
    {
      out.append(frame);
    }

    // Allow one iteration were no progress is made to the outside. 
    // Meaning no frame obtained and no data consumed. Afterward we give up.
    if (!is_frame_available && num_consumed == 0 && !was_data_consumed)
      throw H264DecodeFailure("Cannot decode any more data");

    len -= num_consumed;
    data_in += num_consumed;
    was_data_consumed = num_consumed>0;
  }
  
  return out;
}


py::list PyH264Decoder::flush()
{
    bool is_frame_available = false;
    py::list out;
    do
    {
      ssize_t num_consumed = 0;
      auto frame = decode_frame_impl(nullptr, 0, num_consumed, is_frame_available);
      if (is_frame_available)
      {
        out.append(frame);
      }
    } 
    while (is_frame_available);
    return out;
}


PYBIND11_MODULE(h264decoder, m)
{
  PyEval_InitThreads(); // need for release of the GIL (http://stackoverflow.com/questions/8009613/boost-python-not-supporting-parallelism)

  m.attr("__setattr__")("Frame",create_named_tuple_return_type());

  py::class_<PyH264Decoder>(m, "H264Decoder")
                            .def(py::init<>())
                            .def("decode_frame", &PyH264Decoder::decode_frame)
                            .def("decode", &PyH264Decoder::decode)
                            .def("flush", &PyH264Decoder::flush);
  m.def("disable_logging", disable_logging);
  py::register_exception<H264Exception>(m, "H264Exception");
  py::register_exception<H264InitFailure>(m, "H264InitFailure");
  py::register_exception<H264DecodeFailure>(m, "H264DecodeFailure");
}
