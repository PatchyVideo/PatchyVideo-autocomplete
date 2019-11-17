#pragma once

#include"win32_error.h"

namespace fast_io
{
namespace details
{

struct win32_open_mode
{
DWORD dwDesiredAccess=0,dwShareMode=FILE_SHARE_READ|FILE_SHARE_WRITE;
LPSECURITY_ATTRIBUTES lpSecurityAttributes=nullptr;
DWORD dwCreationDisposition=0;	//depends on EXCL
DWORD dwFlagsAndAttributes=FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS;
};


inline constexpr win32_open_mode calculate_win32_open_mode(open::mode const &om)
{
	using namespace open;
	std::size_t value(remove_ate(om).value);
	win32_open_mode mode;
	if(value&open::app.value)
		mode.dwDesiredAccess|=FILE_APPEND_DATA;
	else if(value&open::out.value)
		mode.dwDesiredAccess|=GENERIC_WRITE;
	if(value&open::in.value)
		mode.dwDesiredAccess|=GENERIC_READ;
	if(value&open::excl.value)
	{
		mode.dwCreationDisposition=CREATE_NEW;
		if(value&open::trunc.value)
			throw std::runtime_error("cannot create new while truncating existed file");
	}
	else if (value&open::trunc.value)
		mode.dwCreationDisposition=CREATE_ALWAYS;
	else if(!(value&open::in.value))
	{
		if(value&open::app.value)
			mode.dwCreationDisposition=OPEN_ALWAYS;
		else
			mode.dwCreationDisposition=CREATE_ALWAYS;
	}
	else
		mode.dwCreationDisposition=OPEN_ALWAYS;
	if(value&open::direct.value)
		mode.dwFlagsAndAttributes|=FILE_FLAG_NO_BUFFERING;
	if(value&open::sync.value)
		mode.dwFlagsAndAttributes|=FILE_FLAG_WRITE_THROUGH;
	return mode;
}
template<std::size_t om>
struct win32_file_openmode
{
	static win32_open_mode constexpr mode = calculate_win32_open_mode(om);
};
}

class win32_io_handle
{
	HANDLE mhandle;
protected:
	void close_impl()
	{
		if(mhandle)
			CloseHandle(mhandle);
	}
	HANDLE& protected_native_handle()	{return mhandle;}
	win32_io_handle() = default;
public:
	using char_type = char;
	using native_handle_type = HANDLE;
	win32_io_handle(native_handle_type handle):mhandle(handle){}
	win32_io_handle(DWORD dw):mhandle(GetStdHandle(dw)){}

	win32_io_handle(win32_io_handle const&)=delete;
	win32_io_handle& operator=(win32_io_handle const&)=delete;
	win32_io_handle(win32_io_handle&& b) noexcept:mhandle(b.mhandle)
	{
		b.mhandle=nullptr;
	}
	win32_io_handle& operator=(win32_io_handle&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			mhandle = b.mhandle;
			b.mhandle=nullptr;
		}
		return *this;
	}
	native_handle_type native_handle()
	{
		return mhandle;
	}
	void flush()
	{
	}
	inline void swap(win32_io_handle& o) noexcept
	{
		using std::swap;
		swap(mhandle,o.mhandle);
	}
};

inline void swap(win32_io_handle& a,win32_io_handle& b) noexcept
{
	a.swap(b);
}

template<typename T,std::integral U>
inline std::common_type_t<long long, std::size_t> seek(win32_io_handle& handle,seek_type_t<T>,U i=0,seekdir s=seekdir::cur)
{
	LARGE_INTEGER distance_to_move_high{};
	LARGE_INTEGER seekposition;
	seekposition.QuadPart=seek_precondition<long long,T,typename win32_io_handle::char_type>(i);
	if(!SetFilePointerEx(handle.native_handle(),seekposition,std::addressof(distance_to_move_high),static_cast<DWORD>(s)))
	{
		auto const last_error(GetLastError());
		if(last_error)
			throw win32_error(last_error);
	}
	return distance_to_move_high.QuadPart;
}

template<std::integral U>
inline auto seek(win32_io_handle& handle,U i=0,seekdir s=seekdir::cur)
{
	return seek(handle,seek_type<typename win32_io_handle::char_type>,i,s);
}

template<std::contiguous_iterator Iter>
inline Iter reads(win32_io_handle& handle,Iter begin,Iter end)
{
	DWORD numberOfBytesRead;
	if(!ReadFile(handle.native_handle(),std::to_address(begin),static_cast<DWORD>((end-begin)*sizeof(*begin)),std::addressof(numberOfBytesRead),nullptr))
		throw win32_error();
	return begin+numberOfBytesRead;
}
template<std::contiguous_iterator Iter>
inline Iter writes(win32_io_handle& handle,Iter cbegin,Iter cend)
{
	auto nNumberOfBytesToWrite(static_cast<DWORD>((cend-cbegin)*sizeof(*cbegin)));
	DWORD numberOfBytesWritten;
	if(!WriteFile(handle.native_handle(),std::to_address(cbegin),nNumberOfBytesToWrite,std::addressof(numberOfBytesWritten),nullptr))
		throw win32_error();
	return cbegin+numberOfBytesWritten/sizeof(*cbegin);
}
inline constexpr void flush(win32_io_handle&){}
class win32_file:public win32_io_handle
{
public:
	using char_type = char;
	using native_handle_type = HANDLE;
	template<typename ...Args>
	win32_file(fast_io::native_interface_t,Args&& ...args):win32_io_handle(CreateFileW(std::forward<Args>(args)...))
	{
		if(native_handle()==INVALID_HANDLE_VALUE)
			throw win32_error();
	}
	template<std::size_t om>
	win32_file(std::string_view filename,open::interface_t<om>):win32_file(fast_io::native_interface,fast_io::utf8_to_ucs<std::wstring>(filename).c_str(),
				details::win32_file_openmode<om>::mode.dwDesiredAccess,
				details::win32_file_openmode<om>::mode.dwShareMode,
				details::win32_file_openmode<om>::mode.lpSecurityAttributes,
				details::win32_file_openmode<om>::mode.dwCreationDisposition,
				details::win32_file_openmode<om>::mode.dwFlagsAndAttributes,nullptr)
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	win32_file(std::string_view filename,open::mode const& m):win32_io_handle(nullptr)
	{
		auto const mode(details::calculate_win32_open_mode(m));
		if((protected_native_handle()=CreateFileW(fast_io::utf8_to_ucs<std::wstring>(filename).c_str(),
					mode.dwDesiredAccess,
					mode.dwShareMode,
					mode.lpSecurityAttributes,
					mode.dwCreationDisposition,
					mode.dwFlagsAndAttributes,nullptr))==INVALID_HANDLE_VALUE)
			throw win32_error();
		if(with_ate(m))
			seek(*this,0,seekdir::end);
	}
	win32_file(std::string_view file,std::string_view mode):win32_file(file,fast_io::open::c_style(mode)){}

	win32_file(win32_file const&)=delete;
	win32_file& operator=(win32_file const&)=delete;
	win32_file(win32_file&& b) noexcept:win32_io_handle(b.protected_native_handle())
	{
		b.protected_native_handle()=nullptr;
	}
	win32_file& operator=(win32_file&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			protected_native_handle() = b.protected_native_handle();
			b.protected_native_handle()=nullptr;
		}
		return *this;
	}
	~win32_file()
	{
		win32_io_handle::close_impl();
	}
};

inline auto zero_copy_in_handle(win32_file& handle)
{
	return handle.native_handle();
}

class win32_pipe_unique:public win32_io_handle
{
public:
	using char_type = char;
	using native_handle_type = HANDLE;
	void close()
	{
		win32_io_handle::close_impl();
		protected_native_handle() = nullptr;
	}
	win32_pipe_unique()=default;
	win32_pipe_unique(win32_pipe_unique const&)=delete;
	win32_pipe_unique& operator=(win32_pipe_unique const&)=delete;
	win32_pipe_unique(win32_pipe_unique&& b) noexcept:win32_io_handle(b.protected_native_handle())
	{
		b.protected_native_handle()=nullptr;
	}
	win32_pipe_unique& operator=(win32_pipe_unique&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			protected_native_handle() = b.protected_native_handle();
			b.protected_native_handle()=nullptr;
		}
		return *this;
	}
	~win32_pipe_unique()
	{
		win32_io_handle::close_impl();
	}
};

class win32_pipe
{
public:
	using char_type = char;
	using native_handle_type = std::array<win32_pipe_unique,2>;
private:
	native_handle_type pipes;
public:
	template<typename ...Args>
	win32_pipe(fast_io::native_interface_t, Args&& ...args)
	{
		if(!::CreatePipe(static_cast<void**>(static_cast<void*>(pipes.data())),static_cast<void**>(static_cast<void*>(pipes.data()+1)),std::forward<Args>(args)...))
			throw win32_error();
	}
	win32_pipe():win32_pipe(fast_io::native_interface,nullptr,0)
	{
	}
	template<std::size_t om>
	win32_pipe(open::interface_t<om>):win32_pipe()
	{
		auto constexpr omb(om&~open::binary.value);
		static_assert(omb==open::in.value||omb==open::out.value||omb==(open::in.value|open::out.value),"pipe open mode must be in or out");
		if constexpr (!(om&~open::in.value)&&(om&~open::out.value))
			pipes.front().close();
		if constexpr ((om&~open::in.value)&&!(om&~open::out.value))
			pipes.back().close();
	}
	auto& native_handle()
	{
		return pipes;
	}
	void flush()
	{
	}
	auto& in()
	{
		return pipes.front();
	}
	auto& out()
	{
		return pipes.back();
	}
	template<std::contiguous_iterator Iter>
	Iter reads(Iter begin,Iter end)
	{
		return reads(pipes.front(),begin,end);
	}
	template<std::contiguous_iterator Iter>
	Iter writes(Iter begin,Iter end)
	{
		return writes(pipes.back(),begin,end);
	}
	void swap(win32_pipe& o) noexcept
	{
		using std::swap;
		swap(pipes,o.pipes);
	}
};

template<std::contiguous_iterator Iter>
inline Iter reads(win32_pipe& h,Iter begin,Iter end)
{
	return reads(h.in(),begin,end);
}
template<std::contiguous_iterator Iter>
inline Iter writes(win32_pipe& h,Iter begin,Iter end)
{
	return writes(h.out(),begin,end);
}

inline constexpr void flush(win32_pipe&){}

using system_file = win32_file;
using system_io_handle = win32_io_handle;
using system_pipe_unique = win32_pipe_unique;
using system_pipe = win32_pipe;

inline constexpr DWORD native_stdin_number(-10);
inline constexpr DWORD native_stdout_number(-11);
inline constexpr DWORD native_stderr_number(-12);

}
