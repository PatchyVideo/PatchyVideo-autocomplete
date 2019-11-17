#pragma once

//fast_io_freestanding.h is usable when the underlining system implements dynamic memory allocations and exceptions

#include"fast_io_core.h"
#include<stdexcept>
#include<string>
#include<bitset>
#include<cmath>

#include"fast_io_freestanding_impl/concat.h"
#include"fast_io_freestanding_impl/manip.h"
#include"fast_io_freestanding_impl/scan_print.h"
#include"fast_io_freestanding_impl/exception.h"
//compile floating point is slow since it requires algorithms like ryu
#include"fast_io_freestanding_impl/floating.h"
#include"fast_io_freestanding_impl/iobuf.h"
#include"fast_io_freestanding_impl/dynamic.h"
#include"fast_io_freestanding_impl/read_write.h"
#include"fast_io_freestanding_impl/natural.h"
#include"fast_io_freestanding_impl/ucs.h"

namespace fast_io
{
using ostring = basic_ostring<std::string>;
}