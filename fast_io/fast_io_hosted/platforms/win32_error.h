#pragma once
#undef min
#undef max
#include<ws2tcpip.h>

#undef interface			//what a joke. Who did this?
#undef min			//what a joke. Who did this?
#undef max			//what a joke. Who did this?

namespace fast_io
{

//void * __stdcall LoadLibraryW(wchar_t const*);

class win32_error : public std::runtime_error
{
	static std::string format_get_last_error(DWORD error)
	{
		if (error)
		{
			char *lpMsgBuf;
			auto bufLen(FormatMessage(
			0x00000100 | 0x00000200 | 0x00001000,
			nullptr,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, nullptr));
			if (bufLen)
			{
				std::unique_ptr<char,decltype(LocalFree)*> up(lpMsgBuf,LocalFree);
				return std::string(up.get(), up.get()+bufLen);
			}
		}
		return std::string();
	}
public:
	explicit win32_error(DWORD const& error = GetLastError()):std::runtime_error(format_get_last_error(error)){}
};
}