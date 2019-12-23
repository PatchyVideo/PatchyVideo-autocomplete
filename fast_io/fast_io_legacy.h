#pragma once

//fast_io_legacy.h deals with legacy C <stdio.h> and C++ <iostream>/<fstream>/<sstream> interface
#include"fast_io_freestanding.h"
#include"fast_io_legacy_impl/c_style.h"
#include"fast_io_legacy_impl/stream_view.h"
#include"fast_io_legacy_impl/streambuf_view.h"

#include<iostream>

namespace fast_io
{
inline namespace
{
inline c_style_io_handle c_stdin(stdin);
inline c_style_io_handle c_stdout(stdout);
inline c_style_io_handle c_stderr(stderr);
/*
inline streambuf_view cpp_cin(std::cin.rdbuf());
inline streambuf_view cpp_cout(std::cout.rdbuf());
inline streambuf_view cpp_cerr(std::cerr.rdbuf());
inline streambuf_view cpp_clog(std::clog.rdbuf());
*/

inline stream_view cpp_cin(std::cin);
inline stream_view cpp_cout(std::cout);
inline stream_view cpp_cerr(std::cerr);
inline stream_view cpp_clog(std::clog);


}
}