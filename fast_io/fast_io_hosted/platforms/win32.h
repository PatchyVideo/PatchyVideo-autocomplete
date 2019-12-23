#pragma once

#include"win32_error.h"

namespace fast_io
{
namespace details
{

struct win32_open_mode
{
std::uint32_t dwDesiredAccess{},dwShareMode=1|2;//FILE_SHARE_READ|FILE_SHARE_WRITE
fast_io::win32::security_attributes *lpSecurityAttributes{nullptr};
std::uint32_t dwCreationDisposition{};	//depends on EXCL
std::uint32_t dwFlagsAndAttributes=128|0x10000000;//FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS
};


inline constexpr win32_open_mode calculate_win32_open_mode(open::mode const &om)
{
	using namespace open;
	std::size_t value(remove_ate(om).value);
	win32_open_mode mode;
	if(value&open::app.value)
		mode.dwDesiredAccess|=4;//FILE_APPEND_DATA
	else if(value&open::out.value)
		mode.dwDesiredAccess|=0x40000000;//GENERIC_WRITE
	if(value&open::in.value)
		mode.dwDesiredAccess|=0x80000000;//GENERIC_READ
	if(value&open::excl.value)
	{
		mode.dwCreationDisposition=1;//	CREATE_NEW
		if(value&open::trunc.value)
			throw std::runtime_error("cannot create new while truncating existed file");
	}
	else if (value&open::trunc.value)
		mode.dwCreationDisposition=2;//CREATE_ALWAYS
	else if(!(value&open::in.value))
	{
		if(value&open::app.value)
			mode.dwCreationDisposition=4;//OPEN_ALWAYS
		else
			mode.dwCreationDisposition=2;//CREATE_ALWAYS
	}
	else
		mode.dwCreationDisposition=3;//OPEN_EXISTING
	if(value&open::direct.value)
		mode.dwFlagsAndAttributes|=0x20000000;//FILE_FLAG_NO_BUFFERING
	if(value&open::sync.value)
		mode.dwFlagsAndAttributes|=0x80000000;//FILE_FLAG_WRITE_THROUGH
	if(value&open::overlapped.value)
		mode.dwFlagsAndAttributes|=0x40000000;//FILE_FLAG_OVERLAPPED
	return mode;
}

inline constexpr std::uint32_t dw_flag_attribute_with_perms(std::uint32_t dw_flags_and_attributes,perms pm)
{
	if((pm&perms::owner_write)==perms::none)
		return dw_flags_and_attributes|1;//dw_flags_and_attributes|FILE_ATTRIBUTE_READONLY
	return dw_flags_and_attributes;
}

inline constexpr win32_open_mode calculate_win32_open_mode_with_perms(open::mode const &om,perms pm)
{
	auto m(calculate_win32_open_mode(om));
	m.dwFlagsAndAttributes=dw_flag_attribute_with_perms(m.dwFlagsAndAttributes,pm);
	return m;
}

template<std::size_t om,perms pm>
struct win32_file_openmode
{
	inline static constexpr win32_open_mode mode = calculate_win32_open_mode_with_perms(om,pm);
};

template<std::size_t om>
struct win32_file_openmode_single
{
	inline static constexpr win32_open_mode mode = calculate_win32_open_mode(om);
};
}

template<std::integral ch_type>
class basic_win32_io_handle
{
public:
	using native_handle_type = void*;
private:
	native_handle_type mhandle;
protected:
	void close_impl()
	{
		if(mhandle)
			fast_io::win32::CloseHandle(mhandle);
	}
	basic_win32_io_handle() = default;
public:
	using char_type = ch_type;
	basic_win32_io_handle(native_handle_type handle):mhandle(handle){}
	basic_win32_io_handle(std::uint32_t dw):mhandle(win32::GetStdHandle(dw)){}

	basic_win32_io_handle(basic_win32_io_handle const& other)
	{
		auto const current_process(win32::GetCurrentProcess());
		if(!win32::DuplicateHandle(current_process,other.mhandle,current_process,std::addressof(mhandle), 0, true, 2/*DUPLICATE_SAME_ACCESS*/))
			throw win32_error();
	}
	basic_win32_io_handle& operator=(basic_win32_io_handle const& other)
	{
		auto const current_process(win32::GetCurrentProcess());
		void* new_handle{};
		if(!win32::DuplicateHandle(current_process,other.mhandle,current_process,std::addressof(new_handle), 0, true, 2/*DUPLICATE_SAME_ACCESS*/))
			throw win32_error();
		mhandle=new_handle;
		return *this;
	}
	basic_win32_io_handle(basic_win32_io_handle&& b) noexcept:mhandle(b.mhandle)
	{
		b.mhandle=nullptr;
	}
	basic_win32_io_handle& operator=(basic_win32_io_handle&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			mhandle = b.mhandle;
			b.mhandle=nullptr;
		}
		return *this;
	}
	native_handle_type& native_handle()
	{
		return mhandle;
	}
	inline void swap(basic_win32_io_handle& o) noexcept
	{
		using std::swap;
		swap(mhandle,o.mhandle);
	}
};

template<std::integral ch_type>
inline void swap(basic_win32_io_handle<ch_type>& a,basic_win32_io_handle<ch_type>& b) noexcept
{
	a.swap(b);
}

template<std::integral ch_type,typename T,std::integral U>
inline std::common_type_t<std::int64_t, std::size_t> seek(basic_win32_io_handle<ch_type>& handle,seek_type_t<T>,U i=0,seekdir s=seekdir::cur)
{
	std::int64_t distance_to_move_high{};
	std::int64_t seekposition{seek_precondition<std::int64_t,T,ch_type>(i)};
	if(!win32::SetFilePointerEx(handle.native_handle(),seekposition,std::addressof(distance_to_move_high),static_cast<std::uint32_t>(s)))
		throw win32_error();
	return distance_to_move_high;
}

template<std::integral ch_type,std::integral U>
inline auto seek(basic_win32_io_handle<ch_type>& handle,U i=0,seekdir s=seekdir::cur)
{
	return seek(handle,seek_type<ch_type>,i,s);
}

template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter receive(basic_win32_io_handle<ch_type>& handle,Iter begin,Iter end)
{
	std::uint32_t numberOfBytesRead;
	if(!win32::ReadFile(handle.native_handle(),std::to_address(begin),static_cast<std::uint32_t>((end-begin)*sizeof(*begin)),std::addressof(numberOfBytesRead),nullptr))
		throw win32_error();
	return begin+numberOfBytesRead;
}
template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter send(basic_win32_io_handle<ch_type>& handle,Iter cbegin,Iter cend)
{
	auto nNumberOfBytesToWrite(static_cast<std::uint32_t>((cend-cbegin)*sizeof(*cbegin)));
	std::uint32_t numberOfBytesWritten;
	if(!win32::WriteFile(handle.native_handle(),std::to_address(cbegin),nNumberOfBytesToWrite,std::addressof(numberOfBytesWritten),nullptr))
		throw win32_error();
	return cbegin+numberOfBytesWritten/sizeof(*cbegin);
}
template<std::integral ch_type>
inline constexpr void flush(basic_win32_io_handle<ch_type>&){}

template<std::integral ch_type>
class basic_win32_file:public basic_win32_io_handle<ch_type>
{
public:
	using char_type=ch_type;
	using native_handle_type = basic_win32_io_handle<ch_type>::native_handle_type;
	using basic_win32_io_handle<ch_type>::native_handle;
	template<typename ...Args>
	requires requires(Args&& ...args)
	{
		{win32::CreateFileW(std::forward<Args>(args)...)}->std::same_as<native_handle_type>;
	}
	basic_win32_file(fast_io::native_interface_t,Args&& ...args):basic_win32_io_handle<char_type>(win32::CreateFileW(std::forward<Args>(args)...))
	{
		if(native_handle()==((void*) (std::intptr_t)-1))
			throw win32_error();
	}

	template<std::size_t om,perms pm>
	basic_win32_file(std::string_view filename,open::interface_t<om>,perms_interface_t<pm>):basic_win32_io_handle<char_type>(win32::CreateFileA(filename.data(),
				details::win32_file_openmode<om,pm>::mode.dwDesiredAccess,
				details::win32_file_openmode<om,pm>::mode.dwShareMode,
				details::win32_file_openmode<om,pm>::mode.lpSecurityAttributes,
				details::win32_file_openmode<om,pm>::mode.dwCreationDisposition,
				details::win32_file_openmode<om,pm>::mode.dwFlagsAndAttributes,nullptr))
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	template<std::size_t om>
	basic_win32_file(std::string_view filename,open::interface_t<om>):basic_win32_io_handle<char_type>(win32::CreateFileA(filename.data(),
				details::win32_file_openmode_single<om>::mode.dwDesiredAccess,
				details::win32_file_openmode_single<om>::mode.dwShareMode,
				details::win32_file_openmode_single<om>::mode.lpSecurityAttributes,
				details::win32_file_openmode_single<om>::mode.dwCreationDisposition,
				details::win32_file_openmode_single<om>::mode.dwFlagsAndAttributes,nullptr))
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	template<std::size_t om>
	basic_win32_file(std::string_view filename,open::interface_t<om>,perms p):basic_win32_io_handle<char_type>(win32::CreateFileA(filename.data(),
				details::win32_file_openmode_single<om>::mode.dwDesiredAccess,
				details::win32_file_openmode_single<om>::mode.dwShareMode,
				details::win32_file_openmode_single<om>::mode.lpSecurityAttributes,
				details::win32_file_openmode_single<om>::mode.dwCreationDisposition,
				details::dw_flag_attribute_with_perms(details::win32_file_openmode_single<om>::mode.dwFlagsAndAttributes,p),nullptr))
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	basic_win32_file(std::string_view filename,open::mode const& m,perms pm=static_cast<perms>(420)):basic_win32_io_handle<char_type>(nullptr)
	{
		auto const mode(details::calculate_win32_open_mode_with_perms(m,pm));
		if((native_handle()=win32::CreateFileA(filename.data(),
					mode.dwDesiredAccess,
					mode.dwShareMode,
					mode.lpSecurityAttributes,
					mode.dwCreationDisposition,
					mode.dwFlagsAndAttributes,nullptr))==((void*) (std::intptr_t)-1))
			throw win32_error();
		if(with_ate(m))
			seek(*this,0,seekdir::end);
	}
	basic_win32_file(std::string_view file,std::string_view mode,perms pm=static_cast<perms>(420)):basic_win32_file(file,fast_io::open::c_style(mode),pm){}
	~basic_win32_file()
	{
		this->close_impl();
	}
};

template<std::integral ch_type>
inline auto zero_copy_in_handle(basic_win32_io_handle<ch_type>& handle)
{
	return handle.native_handle();
}

template<std::integral ch_type>
class basic_win32_pipe_unique:public basic_win32_io_handle<ch_type>
{
public:
	using char_type=ch_type;
	using basic_win32_io_handle<ch_type>::native_handle_type;
	using basic_win32_io_handle<ch_type>::native_handle;
	void close()
	{
		this->close_impl();
		native_handle() = nullptr;
	}
	basic_win32_pipe_unique()=default;
/*	win32_pipe_unique(win32_pipe_unique const&)=delete;
	win32_pipe_unique& operator=(win32_pipe_unique const&)=delete;
	win32_pipe_unique(win32_pipe_unique&& b) noexcept:win32_io_handle(b.native_handle())
	{
		b.native_handle()=nullptr;
	}
	win32_pipe_unique& operator=(win32_pipe_unique&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			native_handle() = b.native_handle();
			b.native_handle()=nullptr;
		}
		return *this;
	}*/
	~basic_win32_pipe_unique()
	{
		this->close_impl();
	}
};

template<std::integral ch_type>
class basic_win32_pipe
{
public:
	using char_type = ch_type;
	using native_handle_type = std::array<basic_win32_pipe_unique<ch_type>,2>;
private:
	native_handle_type pipes;
public:
	template<typename ...Args>
	requires requires(Args&& ...args)
	{
		{win32::CreatePipe(static_cast<void**>(static_cast<void*>(pipes.data())),static_cast<void**>(static_cast<void*>(pipes.data()+1)),std::forward<Args>(args)...)}->std::same_as<int>;
	}
	basic_win32_pipe(fast_io::native_interface_t, Args&& ...args)
	{
		if(!win32::CreatePipe(static_cast<void**>(static_cast<void*>(pipes.data())),static_cast<void**>(static_cast<void*>(pipes.data()+1)),std::forward<Args>(args)...))
			throw win32_error();
	}
	basic_win32_pipe():basic_win32_pipe(fast_io::native_interface,nullptr,0)
	{
	}
	template<std::size_t om>
	basic_win32_pipe(open::interface_t<om>):basic_win32_pipe()
	{
		auto constexpr omb(om&~open::binary.value);
		static_assert(omb==open::in.value||omb==open::out.value||omb==(open::in.value|open::out.value),u8"pipe open mode must be in or out");
		if constexpr (!(om&~open::in.value)&&(om&~open::out.value))
			pipes.front().close();
		if constexpr ((om&~open::in.value)&&!(om&~open::out.value))
			pipes.back().close();
	}
	auto& native_handle()
	{
		return pipes;
	}
	auto& in()
	{
		return pipes.front();
	}
	auto& out()
	{
		return pipes.back();
	}
	void swap(basic_win32_pipe& o) noexcept
	{
		using std::swap;
		swap(pipes,o.pipes);
	}
};

template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter receive(basic_win32_pipe<ch_type>& h,Iter begin,Iter end)
{
	return receive(h.in(),begin,end);
}
template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter send(basic_win32_pipe<ch_type>& h,Iter begin,Iter end)
{
	return send(h.out(),begin,end);
}

template<std::integral ch_type>
inline constexpr void flush(basic_win32_pipe<ch_type>&){}

using win32_io_handle=basic_win32_io_handle<char>;
using win32_file=basic_win32_file<char>;
using win32_pipe_unique=basic_win32_pipe_unique<char>;
using win32_pipe=basic_win32_pipe<char>;

using u8win32_io_handle=basic_win32_io_handle<char8_t>;
using u8win32_file=basic_win32_file<char8_t>;
using u8win32_pipe_unique=basic_win32_pipe_unique<char8_t>;
using u8win32_pipe=basic_win32_pipe<char8_t>;

inline constexpr std::uint32_t win32_stdin_number(-10);
inline constexpr std::uint32_t win32_stdout_number(-11);
inline constexpr std::uint32_t win32_stderr_number(-12);


inline win32_io_handle win32_stdin()
{
	return win32_stdin_number;
}

inline win32_io_handle win32_stdout()
{
	return win32_stdout_number;
}

inline win32_io_handle win32_stderr()
{
	return win32_stderr_number;
}

}
