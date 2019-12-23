#pragma once

#include"win32apis.h"

namespace fast_io
{

//void * __stdcall LoadLibraryW(wchar_t const*);


class win32_error : public std::runtime_error
{
	inline static std::string format_get_last_error(std::uint32_t error)
	{
		if (error)
		{
			std::array<char,32768> buffer;
			auto const buffer_length(win32::FormatMessageA(
			0x00000200 | 0x00001000,
			nullptr,
			error,
			(1 << 10),
			buffer.data(),
			buffer.size(),
			nullptr));
			if (buffer_length)
				return std::string(buffer.data(),buffer.data()+buffer_length);
		}
		return {};
	}
	std::uint32_t ec;
public:
	explicit win32_error(std::uint32_t error = win32::GetLastError()):std::runtime_error(format_get_last_error(error)),ec(error){}
	auto get() const noexcept
	{
		return ec;
	}
};
}