#pragma once
#include<unistd.h>
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
	std::size_t value(remove_ate(om).value);
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
		mode |= O_CREAT  | O_EXCL;
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
		throw std::runtime_error("unknown posix file openmode");
	}
}
template<std::size_t om>
struct posix_file_openmode
{
	static int constexpr mode = calculate_posix_open_mode(om);
};
}

class posix_io_handle
{
	int fd;
protected:
	void close_impl() noexcept
	{
		if(fd!=-1)
			close(fd);
	}
	auto& protected_native_handle()
	{
		return fd;
	}
public:
	using char_type = char;
	using native_handle_type = int;
	native_handle_type native_handle()
	{
		return fd;
	}
	posix_io_handle() = default;
	posix_io_handle(int fdd):fd(fdd){}
	posix_io_handle(posix_io_handle const& dp):fd(dup(dp.fd))
	{
		if(fd<0)
			throw std::system_error(errno,std::generic_category());
	}
	posix_io_handle& operator=(posix_io_handle const& dp)
	{
		auto newfd(dup2(dp.fd,fd));
		if(newfd<0)
			throw std::system_error(errno,std::generic_category());
		fd=newfd;
		return *this;
	}
	posix_io_handle(posix_io_handle&& b) noexcept : posix_io_handle(b.fd)
	{
		b.fd = -1;
	}
	posix_io_handle& operator=(posix_io_handle&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			fd=b.fd;
			b.fd = -1;
		}
		return *this;
	}
	void swap(posix_io_handle& o) noexcept
	{
		using std::swap;
		swap(fd,o.fd);
	}
};

template<std::contiguous_iterator Iter>
inline Iter reads(posix_io_handle& h,Iter begin,Iter end)
{
	auto read_bytes(::read(h.native_handle(),std::to_address(begin),(end-begin)*sizeof(*begin)));
	if(read_bytes==-1)
		throw std::system_error(errno,std::generic_category());
	return begin+(read_bytes/sizeof(*begin));
}
template<std::contiguous_iterator Iter>
inline Iter writes(posix_io_handle& h,Iter begin,Iter end)
{
	auto write_bytes(::write(h.native_handle(),std::to_address(begin),(end-begin)*sizeof(*begin)));
	if(write_bytes==-1)
		throw std::system_error(errno,std::generic_category());
	return begin+(write_bytes/sizeof(*begin));
}

template<typename T,std::integral R>
inline std::common_type_t<off64_t, std::size_t> seek(posix_io_handle& h,seek_type_t<T>,R i=0,seekdir s=seekdir::cur)
{
	auto ret(::lseek64(h.native_handle(),seek_precondition<off64_t,T,posix_io_handle::char_type>(i),static_cast<int>(s)));
	if(ret==-1)
		throw std::system_error(errno,std::generic_category());
	return ret;
}
template<std::integral R>
inline auto seek(posix_io_handle& h,R i=0,seekdir s=seekdir::cur)
{
	return seek(h,seek_type<posix_io_handle::char_type>,i,s);
}
inline void flush(posix_io_handle&)
{
	// no need fsync. OS can deal with it
//		if(::fsync(fd)==-1)
//			throw std::system_error(errno,std::generic_category());
}

#ifdef __linux__
inline auto zero_copy_in_handle(posix_io_handle& h)
{
	return h.native_handle();
}
inline auto zero_copy_out_handle(posix_io_handle& h)
{
	return h.native_handle();
}
#endif

inline void swap(posix_io_handle& a,posix_io_handle& b) noexcept
{
	a.swap(b);
}

class posix_file:public posix_io_handle
{
public:
	using char_type = posix_io_handle::char_type;
	using native_handle_type = posix_io_handle::native_handle_type;
	template<typename ...Args>
	posix_file(native_interface_t,Args&& ...args):posix_io_handle(::open(std::forward<Args>(args)...))
	{
		if(native_handle()==-1)
			throw std::system_error(errno,std::generic_category());
	}
	template<std::size_t om>
	posix_file(std::string_view file,open::interface_t<om>):posix_file(native_interface,file.data(),details::posix_file_openmode<om>::mode,420)
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	//potential support modification prv in the future
	posix_file(std::string_view file,open::mode const& m):posix_file(native_interface,file.data(),details::calculate_posix_open_mode(m),420)
	{
		if(with_ate(m))
			seek(*this,0,seekdir::end);
	}
	posix_file(std::string_view file,std::string_view mode):posix_file(file,fast_io::open::c_style(mode)){}
	~posix_file()
	{
		posix_io_handle::close_impl();
	}
};

class posix_pipe_unique:public posix_io_handle
{
public:
	using char_type = char;
	using native_handle_type = int;
	void close()
	{
		posix_io_handle::close_impl();
		protected_native_handle() = -1;
	}
	~posix_pipe_unique()
	{
		posix_io_handle::close_impl();
	}
};

class posix_pipe
{
public:
	using char_type = char;
	using native_handle_type = std::array<posix_pipe_unique,2>;
private:
	native_handle_type pipes;
public:
	posix_pipe()
	{
#ifdef _WIN32_WINNT
		if(_pipe(static_cast<int*>(static_cast<void*>(pipes.data())),1048576,_O_BINARY)==-1)
#else
		if(::pipe(static_cast<int*>(static_cast<void*>(pipes.data())))==-1)
#endif
			throw std::system_error(errno,std::generic_category());
	}
	template<std::size_t om>
	posix_pipe(open::interface_t<om>):posix_pipe()
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
	void swap(posix_pipe& o) noexcept
	{
		using std::swap;
		swap(pipes,o.pipes);
	}
};
inline void swap(posix_pipe& a,posix_pipe& b) noexcept
{
	a.swap(b);
}

template<std::contiguous_iterator Iter>
inline Iter reads(posix_pipe& h,Iter begin,Iter end)
{
	return reads(h.in(),begin,end);
}
template<std::contiguous_iterator Iter>
inline Iter writes(posix_pipe& h,Iter begin,Iter end)
{
	return writes(h.out(),begin,end);
}

inline void flush(posix_pipe&)
{
	// no need fsync. OS can deal with it
//		if(::fsync(fd)==-1)
//			throw std::system_error(errno,std::generic_category());
}

#ifdef __linux__
inline auto zero_copy_in_handle(posix_pipe& h)
{
	return h.in().native_handle();
}
inline auto zero_copy_out_handle(posix_pipe& h)
{
	return h.out().native_handle();
}
#endif



#ifndef __WINNT__
using system_file = posix_file;
using system_io_handle = posix_io_handle;
using system_pipe_unique = posix_pipe_unique;
using system_pipe = posix_pipe;
inline int constexpr native_stdin_number = 0;
inline int constexpr native_stdout_number = 1;
inline int constexpr native_stderr_number = 2;
#endif
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

}
