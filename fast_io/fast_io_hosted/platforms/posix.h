#pragma once

#if defined(__WINNT__) || defined(_MSC_VER)
#include<io.h>
#else
#include<unistd.h>
#endif
#include<fcntl.h>
#ifdef __linux__
#include<sys/sendfile.h>
#endif

namespace fast_io
{
	
namespace details
{
inline constexpr int calculate_posix_open_mode(open::mode const &om)
{
	using namespace open;
	std::size_t value(remove_ate_overlapped(om).value);
	int mode(0);
	if(value&binary.value)
	{
#ifdef O_BINARY
		mode |= O_BINARY;
#endif
		value &= ~binary.value;
	}
	if(value&excl.value)
	{
		mode |= O_CREAT | O_EXCL;
		value &= ~excl.value;
	}
	if(value&trunc.value)
	{
		mode |= O_TRUNC;
		value &= ~trunc.value;
	}
	if(value&direct.value)
	{
#ifdef O_DIRECT
		mode |= O_DIRECT;
#endif
		value &= ~direct.value;
	}
	if(value&sync.value)
	{
#ifdef O_SYNC
		mode |= O_SYNC;
#endif
		value &= ~sync.value;
	}
	switch(value)
	{
//Action if file already exists;	Action if file does not exist;	c-style mode;	Explanation
//Read from start;	Failure to open;	"r";	Open a file for reading
	case in:
		return mode | O_RDONLY;
//Destroy contents;	Create new;	"w";	Create a file for writing
	case out:
		return mode | O_WRONLY | O_CREAT | O_TRUNC;
//Append to file;	Create new;	"a";	Append to a file
	case app:
	case out|app:
		return mode | O_WRONLY | O_CREAT | O_APPEND;
//Read from start;	Error;	"r+";		Open a file for read/write
	case out|in:
		return mode | O_RDWR;
//Write to end;	Create new;	"a+";	Open a file for read/write
	case out|in|app:
	case in|app:
		return mode | O_RDWR | O_CREAT | O_APPEND;
//Destroy contents;	Error;	"wx";	Create a file for writing
	default:
		throw std::runtime_error(reinterpret_cast<char const*>(u8"unknown posix file openmode"));
	}
}
template<std::size_t om>
struct posix_file_openmode
{
	static int constexpr mode = calculate_posix_open_mode(om);
};
}

template<std::integral ch_type>
class basic_posix_io_handle
{
	int fd;
protected:
	void close_impl() noexcept
	{
		if(fd!=-1)
			close(fd);
	}
public:
	using char_type = ch_type;
	using native_handle_type = int;
	native_handle_type& native_handle()
	{
		return fd;
	}
	constexpr basic_posix_io_handle() = default;
	constexpr basic_posix_io_handle(int fdd):fd(fdd){}
	basic_posix_io_handle(basic_posix_io_handle const& dp):fd(dup(dp.fd))
	{
		if(fd<0)
			throw std::system_error(errno,std::generic_category());
	}
	basic_posix_io_handle& operator=(basic_posix_io_handle const& dp)
	{
		auto newfd(dup2(dp.fd,fd));
		if(newfd<0)
			throw std::system_error(errno,std::generic_category());
		fd=newfd;
		return *this;
	}
	constexpr basic_posix_io_handle(basic_posix_io_handle&& b) noexcept : basic_posix_io_handle(b.fd)
	{
		b.fd = -1;
	}
	basic_posix_io_handle& operator=(basic_posix_io_handle&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			fd=b.fd;
			b.fd = -1;
		}
		return *this;
	}
#if defined(__WINNT__) || defined(_MSC_VER)
	explicit operator basic_win32_io_handle<char_type>() const
	{
		return static_cast<basic_win32_io_handle<char_type>>(_get_osfhandle(fd));
	}
#endif
	constexpr void swap(basic_posix_io_handle& o) noexcept
	{
		using std::swap;
		swap(fd,o.fd);
	}
};

template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter receive(basic_posix_io_handle<ch_type>& h,Iter begin,Iter end)
{
	auto read_bytes(::read(h.native_handle(),std::to_address(begin),(end-begin)*sizeof(*begin)));
	if(read_bytes==-1)
		throw std::system_error(errno,std::generic_category());
	return begin+(read_bytes/sizeof(*begin));
}
template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter send(basic_posix_io_handle<ch_type>& h,Iter begin,Iter end)
{
	auto write_bytes(::write(h.native_handle(),std::to_address(begin),(end-begin)*sizeof(*begin)));
	if(write_bytes==-1)
		throw std::system_error(errno,std::generic_category());
	return begin+(write_bytes/sizeof(*begin));
}

template<std::integral ch_type,typename T,std::integral R>
inline std::common_type_t<off64_t, std::size_t> seek(basic_posix_io_handle<ch_type>& h,seek_type_t<T>,R i=0,seekdir s=seekdir::cur)
{
	auto ret(::lseek64(h.native_handle(),seek_precondition<off64_t,T,basic_posix_io_handle<ch_type>::char_type>(i),static_cast<int>(s)));
	if(ret==-1)
		throw std::system_error(errno,std::generic_category());
	return ret;
}
template<std::integral ch_type,std::integral R>
inline auto seek(basic_posix_io_handle<ch_type>& h,R i=0,seekdir s=seekdir::cur)
{
	return seek(h,seek_type<basic_posix_io_handle<ch_type>::char_type>,i,s);
}
template<std::integral ch_type>
inline void flush(basic_posix_io_handle<ch_type>&)
{
	// no need fsync. OS can deal with it
//		if(::fsync(fd)==-1)
//			throw std::system_error(errno,std::generic_category());
}

#ifdef __linux__
template<std::integral ch_type>
inline auto zero_copy_in_handle(basic_posix_io_handle<ch_type>& h)
{
	return h.native_handle();
}
template<std::integral ch_type>
inline auto zero_copy_out_handle(basic_posix_io_handle<ch_type>& h)
{
	return h.native_handle();
}
#endif

template<std::integral ch_type>
inline void swap(basic_posix_io_handle<ch_type>& a,basic_posix_io_handle<ch_type>& b) noexcept
{
	a.swap(b);
}

template<std::integral ch_type>
class basic_posix_file:public basic_posix_io_handle<ch_type>
{
#if defined(__WINNT__) || defined(_MSC_VER)
	using mode_t = int;
#endif
public:
	using char_type = ch_type;
	using native_handle_type = basic_posix_io_handle<char_type>::native_handle_type;
	using basic_posix_io_handle<ch_type>::native_handle;
	template<typename ...Args>
	requires requires(Args&& ...args)
	{
		{
#if defined(__WINNT__) || defined(_MSC_VER)
			::_open
#else
			::open
#endif
(std::forward<Args>(args)...)}->std::same_as<int>;
	}
	basic_posix_file(native_interface_t,Args&& ...args):basic_posix_io_handle<ch_type>(
#if defined(__WINNT__) || defined(_MSC_VER)
			::_open
#else
			::open
#endif
(std::forward<Args>(args)...))
	{
		if(native_handle()==-1)
			throw std::system_error(errno,std::generic_category());
	}
	template<std::size_t om,perms pm>
	basic_posix_file(std::string_view file,open::interface_t<om>,perms_interface_t<pm>):basic_posix_file(native_interface,file.data(),details::posix_file_openmode<om>::mode,static_cast<mode_t>(pm))
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	template<std::size_t om>
	basic_posix_file(std::string_view file,open::interface_t<om>):basic_posix_file(native_interface,file.data(),details::posix_file_openmode<om>::mode,static_cast<mode_t>(420))
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	template<std::size_t om>
	basic_posix_file(std::string_view file,open::interface_t<om>,perms pm):basic_posix_file(native_interface,file.data(),details::posix_file_openmode<om>::mode,static_cast<mode_t>(pm))
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	//potential support modification prv in the future
	basic_posix_file(std::string_view file,open::mode const& m,perms pm=static_cast<perms>(420)):basic_posix_file(native_interface,file.data(),details::calculate_posix_open_mode(m),static_cast<mode_t>(pm))
	{
		if(with_ate(m))
			seek(*this,0,seekdir::end);
	}
	basic_posix_file(std::string_view file,std::string_view mode,perms pm=static_cast<perms>(420)):basic_posix_file(file,fast_io::open::c_style(mode),pm){}
	~basic_posix_file()
	{
		this->close_impl();
	}
};
template<std::integral ch_type>
class basic_posix_pipe_unique:public basic_posix_io_handle<ch_type>
{
public:
	using char_type=ch_type;
	using native_handle_type = int;
	using basic_posix_io_handle<ch_type>::native_handle;
	void close()
	{
		this->close_impl();
		native_handle() = -1;
	}
	~basic_posix_pipe_unique()
	{
		this->close_impl();
	}
};

template<std::integral ch_type>
class basic_posix_pipe
{
public:
	using char_type = ch_type;
	using native_handle_type = std::array<basic_posix_pipe_unique<ch_type>,2>;
private:
	native_handle_type pipes;
public:
	basic_posix_pipe()
	{
#ifdef _WIN32_WINNT
		if(_pipe(static_cast<int*>(static_cast<void*>(pipes.data())),1048576,_O_BINARY)==-1)
#else
		if(::pipe(static_cast<int*>(static_cast<void*>(pipes.data())))==-1)
#endif
			throw std::system_error(errno,std::generic_category());
	}
	template<std::size_t om>
	basic_posix_pipe(open::interface_t<om>):basic_posix_pipe()
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
	void swap(basic_posix_pipe& o) noexcept
	{
		using std::swap;
		swap(pipes,o.pipes);
	}
};
template<std::integral ch_type>
inline void swap(basic_posix_pipe<ch_type>& a,basic_posix_pipe<ch_type>& b) noexcept
{
	a.swap(b);
}

template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter receive(basic_posix_pipe<ch_type>& h,Iter begin,Iter end)
{
	return receive(h.in(),begin,end);
}
template<std::integral ch_type,std::contiguous_iterator Iter>
inline Iter send(basic_posix_pipe<ch_type>& h,Iter begin,Iter end)
{
	return send(h.out(),begin,end);
}

template<std::integral ch_type>
inline void flush(basic_posix_pipe<ch_type>&)
{
	// no need fsync. OS can deal with it
//		if(::fsync(fd)==-1)
//			throw std::system_error(errno,std::generic_category());
}

#ifdef __linux__
template<std::integral ch_type>
inline auto zero_copy_in_handle(basic_posix_pipe<ch_type>& h)
{
	return h.in().native_handle();
}
template<std::integral ch_type>
inline auto zero_copy_out_handle(basic_posix_pipe<ch_type>& h)
{
	return h.out().native_handle();
}
#endif

using posix_io_handle=basic_posix_io_handle<char>;
using posix_file=basic_posix_file<char>;
using posix_pipe_unique=basic_posix_pipe_unique<char>;
using posix_pipe=basic_posix_pipe<char>;

using u8posix_io_handle=basic_posix_io_handle<char8_t>;
using u8posix_file=basic_posix_file<char8_t>;
using u8posix_pipe_unique=basic_posix_pipe_unique<char8_t>;
using u8posix_pipe=basic_posix_pipe<char8_t>;

inline int constexpr posix_stdin_number = 0;
inline int constexpr posix_stdout_number = 1;
inline int constexpr posix_stderr_number = 2;
#ifdef __linux__

//zero copy IO for linux
namespace details
{
template<zero_copy_output_stream output,zero_copy_input_stream input>
inline std::size_t zero_copy_transmit_once(output& outp,input& inp,std::size_t bytes)
{
	auto transmitted_bytes(::sendfile(zero_copy_out_handle(outp),zero_copy_in_handle(inp),nullptr,bytes));
	if(transmitted_bytes==-1)
		throw std::system_error(errno,std::generic_category());
	return transmitted_bytes;
}
}


template<zero_copy_output_stream output,zero_copy_input_stream input>
inline std::size_t zero_copy_transmit(output& outp,input& inp,std::size_t bytes)
{
	std::size_t constexpr maximum_transmit_bytes(2147479552);
	std::size_t transmitted(0);
	for(;bytes;)
	{
		std::size_t should_transfer(maximum_transmit_bytes);
		if(bytes<should_transfer)
			should_transfer=bytes;
		std::size_t transferred_this_round(details::zero_copy_transmit_once(outp,inp,should_transfer));
		transmitted+=transferred_this_round;
		if(transferred_this_round!=should_transfer)
			return transmitted;
		bytes-=transferred_this_round;
	}
	return transmitted;
	
}
template<zero_copy_output_stream output,zero_copy_input_stream input>
inline std::size_t zero_copy_transmit(output& outp,input& inp)
{
	std::size_t constexpr maximum_transmit_bytes(2147479552);
	for(std::size_t transmitted(0);;)
	{
		std::size_t transferred_this_round(details::zero_copy_transmit_once(outp,inp,maximum_transmit_bytes));
		transmitted+=transferred_this_round;
		if(transferred_this_round!=maximum_transmit_bytes)
			return transmitted;
	}
}
#endif

inline constexpr posix_io_handle posix_stdin()
{
	return posix_stdin_number;
}
inline constexpr posix_io_handle posix_stdout()
{
	return posix_stdout_number;
} 
inline constexpr posix_io_handle posix_stderr()
{
	return posix_stderr_number;
}

}
