#pragma once
#include"fast_io_hosted.h"
//This header file defines in/out/err/log like traditional C/C++ does

namespace fast_io
{
inline basic_obuf<system_io_handle> out(native_stdout_number);
inline tie<basic_ibuf<system_io_handle>,decltype(out)> in(out,native_stdin_number);
inline tie<system_io_handle,decltype(out)> err(out,native_stderr_number);
inline basic_obuf<system_io_handle> log(native_stderr_number);
}