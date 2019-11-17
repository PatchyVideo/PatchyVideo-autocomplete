#include<iostream>

namespace fast_io
{

inline streambuf_view in(std::cin.rdbuf());
inline streambuf_view out(std::cout.rdbuf());
inline streambuf_view err(std::cerr.rdbuf());
inline streambuf_view log(std::clog.rdbuf());
}