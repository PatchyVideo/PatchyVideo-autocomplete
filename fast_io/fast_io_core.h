#pragma once

//fast_io_core.h is required to be usable in freestanding environment with EVEN dynamic memory allocation and exceptions are disabled.

#include<version>
#include<cstddef>
#include<limits>
#include<type_traits>
#include<concepts>
#include<cstdint>

//I know functions like memcpy would probably not be provided in freestanding environment. However, you can implement them on these platforms.


//The C++20 standard does not require these headers to be freestanding.
//However, they are clearly necessary and usable even in freestanding environment.
//Leaving no room for a lower-level language: A C++ Subset
//http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1105r0.html
#include<utility>		//for std::move, std::forward, std::addressof
#include<memory>		//std::addressof
#include<array>
#include<string_view>
#include<iterator>		//for iterator concepts
#include<ranges>		//for ranges concepts
#include<algorithm>		//std::fill std::fill_n std::copy std::copy_n
#include<cstring>		//for memcpy

//fast_io core
#include"fast_io_core_impl/concept.h"
#include"fast_io_core_impl/eof.h"
#include"fast_io_core_impl/fill_nc.h"
#include"fast_io_core_impl/io_ref.h"
#include"fast_io_core_impl/manip.h"

// This should provide an option macro to disable any generation for table in freestanding environments.
#include"fast_io_core_impl/shared_base_table.h"
#include"fast_io_core_impl/base.h"
#include"fast_io_core_impl/mode.h"
#include"fast_io_core_impl/istring_view.h"
#include"fast_io_core_impl/ospan.h"
// Provide template prototype for custom string types
#include"fast_io_core_impl/ostring.h"
#include"fast_io_core_impl/istack.h"

// Precondition should be removed in the future. Programming bugs
#include"fast_io_core_impl/precondition.h"
#include"fast_io_core_impl/print_scan.h"
#include"fast_io_core_impl/seek.h"
#include"fast_io_core_impl/sync.h"
#include"fast_io_core_impl/text.h"
#include"fast_io_core_impl/tie.h"
#include"fast_io_core_impl/transmit.h"
#include"fast_io_core_impl/unsigned_extension.h"
#include"fast_io_core_impl/wrapper.h"
#include"fast_io_core_impl/flush.h"