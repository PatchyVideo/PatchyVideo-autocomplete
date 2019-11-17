#pragma once

#if defined(__WINNT__) || defined(_MSC_VER)
#include"win32.h"
#else
#include"posix.h"
#endif

namespace fast_io
{
inline system_io_handle native_stdin()
{
	return native_stdin_number;
} 
inline system_io_handle native_stdout()
{
	return native_stdout_number;
} 
inline system_io_handle native_stderr()
{
	return native_stderr_number;
} 
}