#pragma once
//Referenced from : https://github.com/ned14/status-code/blob/master/include/win32_code.hpp

namespace fast_io::win32
{
extern "C"
{
std::uint32_t __stdcall GetLastError(void);
void * __stdcall LoadLibraryW(wchar_t const*);

// Used to retrieve a locale-specific message string for some error code
std::uint32_t __stdcall FormatMessageW(std::uint32_t, void const*, std::uint32_t,std::uint32_t, wchar_t*, std::uint32_t, void /*va_list*/ *);

struct security_attributes
{
	std::uint32_t nLength;
	void* lpSecurityDescriptor;
	int bInheritHandle;
};

int __stdcall CloseHandle(void*);

void* __stdcall CreateFileW(wchar_t const*,std::uint32_t,std::uint32_t,security_attributes*,std::uint32_t,std::uint32_t,void*);

void* __stdcall CreateFileW(wchar_t const*,std::uint32_t,std::uint32_t,security_attributes*,std::uint32_t,std::uint32_t,void*);

struct overlapped
{
std::uintptr_t Internal;
std::uintptr_t InternalHigh;
union
{
struct
{
std::uint32_t Offset;
std::uint32_t OffsetHigh;
} DUMMYSTRUCTNAME;
void* Pointer;
}
DUMMYUNIONNAME;
void* hEvent;
};
struct transmit_file_buffer
{
void* Head;
std::uint32_t  HeadLength;
void* Tail;
std::uint32_t  TailLength;
};

int __stdcall WriteFile(void*,void const*,std::uint32_t,std::uint32_t*,overlapped*);

int __stdcall ReadFile(void*,void const*,std::uint32_t,std::uint32_t*,overlapped*);

int __stdcall SetFilePointerEx(void*,std::int64_t,std::int64_t*,std::uint32_t);

void* __stdcall GetCurrentProcess();

int __stdcall DuplicateHandle(void*,void*,void*,void**,std::uint32_t,int,std::uint32_t);

void* __stdcall GetStdHandle(std::uint32_t);

int __stdcall CreatePipe(void**,void**,security_attributes*,std::uint32_t);

int __stdcall FreeLibrary(void*);

int __stdcall TransmitFile(std::uintptr_t,void*,std::uint32_t,std::uint32_t,overlapped*,transmit_file_buffer*,std::uint32_t);

using farproc = intptr_t(__stdcall*)();

farproc __stdcall GetProcAddress(void*,char const*);
}

}