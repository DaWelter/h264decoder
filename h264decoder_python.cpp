#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cassert>

// Boost python might need a patch. See https://github.com/boostorg/python/issues/193

// python string api, see
// https://docs.python.org/2/c-api/string.html
extern "C" {
  #include <Python.h>
}

#include <boost/python.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
namespace py = boost::python;
#define IS_PYTHON3 (PY_MAJOR_VERSION>=3)

#include "h264decoder.hpp"

namespace {

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

/* Fetch the pointer from string type (Python  2) or bytes type (Python 3) */
void extract_data_ptr_from_buffer(py::object o, const ubyte* &data_in, ssize_t &len);
}


/* The class wrapped in python via boost::python */
class PyH264Decoder
{
  H264Decoder decoder;
  ConverterRGB24 converter;

  /* Extract frames from input stream. Stops at frame boundaries and returns the number of consumed bytes
   * in num_consumed.
   * 
   * If a frame is completed, is_frame_available is set to true, and the returned python tuple contains
   * formation about the frame as well as the frame buffer memory. 
   * 
   * Else, i.e. all data in the buffer is consumed, is_frame_available is set to false. The returned tuple
   * contains dummy data.
   */ 
  py::tuple decode_frame_impl(const ubyte *data, ssize_t num, ssize_t &num_consumed, bool &is_frame_available);
  
public:
  /* Decoding style analogous to c/c++ way. Stop at frame boundaries. 
   * Return tuple containing frame data as above as nested tuple, and an integer telling how many bytes were consumed.  */
  py::tuple decode_frame(const py::object &py_data_in);
  /* Process all the input data and return a list of all contained frames. */
  py::list  decode(const py::object &py_data_in);
};


py::tuple PyH264Decoder::decode_frame_impl(const ubyte *data_in, ssize_t len, ssize_t &num_consumed, bool &is_frame_available)
{
  GILScopedReverseLock gilguard;
  num_consumed = decoder.parse((ubyte*)data_in, len);
  
  if (is_frame_available = decoder.is_frame_available())
  {
    const auto &frame = decoder.decode_frame();
    int w, h; std::tie(w,h) = width_height(frame);
    Py_ssize_t out_size = converter.predict_size(w,h);

    gilguard.lock();

  // Allocate storage for the output frame.
#if IS_PYTHON3
  py::object py_out_str(py::handle<>(PyBytes_FromStringAndSize(nullptr, out_size)));
  char* out_buffer = PyBytes_AsString(py_out_str.ptr());
#else
    //   Construction of py::handle causes ... TODO: WHAT? No increase of ref count ?!
    py::object py_out_str(py::handle<>(PyString_FromStringAndSize(NULL, out_size)));
    char* out_buffer = PyString_AsString(py_out_str.ptr());
#endif

    gilguard.unlock();
    const auto &rgbframe = converter.convert(frame, (ubyte*)out_buffer);
    
    gilguard.lock();
    return py::make_tuple(py_out_str, w, h, row_size(rgbframe));
  }
  else
  {
    gilguard.lock();
    return py::make_tuple(py::object(), 0, 0, 0);
  }
}


py::tuple PyH264Decoder::decode_frame(const py::object &py_data_in)
{
  ssize_t len;
  const ubyte* data_in;
  extract_data_ptr_from_buffer(py_data_in, data_in, len);

  ssize_t num_consumed = 0;
  bool is_frame_available = false;
  auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
  
  return py::make_tuple(frame, num_consumed);
}


py::list PyH264Decoder::decode(const py::object &py_data_in)
{
  ssize_t len;
  const ubyte* data_in;
  extract_data_ptr_from_buffer(py_data_in, data_in, len);
  
  py::list out;
  
  try
  {
    while (len > 0)
    {
      ssize_t num_consumed = 0;
      bool is_frame_available = false;
      
      try
      {
        auto frame = decode_frame_impl(data_in, len, num_consumed, is_frame_available);
        if (is_frame_available)
        {
          out.append(frame);
        }
      }
      catch (const H264DecodeFailure &e)
      {
        if (num_consumed <= 0)
          // This case is fatal because we cannot continue to move ahead in the stream.
          throw e;
      }
      
      len -= num_consumed;
      data_in += num_consumed;
    }
  }
  catch (const H264DecodeFailure &)
  {
  }
  
  return out;
}


BOOST_PYTHON_MODULE(libh264decoder)
{
  PyEval_InitThreads(); // need for release of the GIL (http://stackoverflow.com/questions/8009613/boost-python-not-supporting-parallelism)
  py::class_<PyH264Decoder>("H264Decoder")
                            .def("decode_frame", &PyH264Decoder::decode_frame)
                            .def("decode", &PyH264Decoder::decode);
  py::def("disable_logging", disable_logging);
}


namespace {
  void extract_data_ptr_from_buffer(py::object o, const ubyte* &data_in, ssize_t &len)
  {
#if IS_PYTHON3
    auto raw_ptr = o.ptr();
    if (!PyBytes_Check(raw_ptr))
      throw std::invalid_argument("Input must be a byte array");
    len = PyBytes_Size(raw_ptr);
    data_in = (const ubyte*)(PyBytes_AsString(raw_ptr));
#else
    if (!PyString_Check(o.ptr()))
      throw std::invalid_argument("Input must be a string");
    len = PyString_Size(o.ptr());
    data_in = (const ubyte*)(PyString_AsString(o.ptr()));
#endif
  }
}
