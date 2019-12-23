#pragma once

#if defined(__WINNT__) || defined(_MSC_VER)
#include"win32.h"
#endif
#include"posix.h"

namespace fast_io
{

#if defined(__WINNT__) || defined(_MSC_VER)
inline constexpr std::uint32_t native_stdin_number(win32_stdin_number);
inline constexpr std::uint32_t native_stdout_number(win32_stdout_number);
inline constexpr std::uint32_t native_stderr_number(win32_stderr_number);
using system_file = win32_file;
using system_io_handle = win32_io_handle;
using system_pipe_unique = win32_pipe_unique;
using system_pipe = win32_pipe;
using u8system_file = u8win32_file;
using u8system_io_handle = u8win32_io_handle;
using u8system_pipe_unique = u8win32_pipe_unique;
using u8system_pipe = u8win32_pipe;


#else
inline constexpr std::uint32_t native_stdin_number(posix_stdin_number);
inline constexpr std::uint32_t native_stdout_number(posix_stdout_number);
inline constexpr std::uint32_t native_stderr_number(posix_stderr_number);

using system_file = posix_file;
using system_io_handle = posix_io_handle;
using system_pipe_unique = posix_pipe_unique;
using system_pipe = posix_pipe;
using u8system_file = u8posix_file;
using u8system_io_handle = u8posix_io_handle;
using u8system_pipe_unique = u8posix_pipe_unique;
using u8system_pipe = u8posix_pipe;

#endif


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