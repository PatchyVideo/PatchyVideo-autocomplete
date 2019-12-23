#pragma once
#include<cstdio>

namespace fast_io
{

class c_style_io_handle_unlocked
{
	std::FILE *fp;
protected:
	auto& protected_native_handle()
	{
		return fp;
	}
public:
	c_style_io_handle_unlocked(std::FILE* fpp):fp(fpp){}
	using char_type = char;
	using native_handle_type = std::FILE*;
	native_handle_type native_handle() const
	{
		return fp;
	}
};

inline auto fileno(c_style_io_handle_unlocked& handle)
{
#if defined(__WINNT__) || defined(_MSC_VER)
	return _fileno(handle.native_handle());
#else
	return ::fileno_unlocked(handle.native_handle());
#endif
}

#if defined(__WINNT__) || defined(_MSC_VER)
/*
#ifdef _MSC_VER

struct _iobuf
{
char *_ptr;
int _cnt;
char *_base;
int _flag;
int _file;
int _charbuf;
int _bufsiz;
char *_tmpfname;
};
#endif
*/
template<std::contiguous_iterator Iter>
inline Iter receive(c_style_io_handle_unlocked& cfhd,Iter begin,Iter end)
{
	std::size_t const count(end-begin);
	std::size_t const r(
//mingw
#ifdef _MSC_VER
	_fread_nolock
#else
	fread
#endif
	(std::to_address(begin),sizeof(*begin),count,cfhd.native_handle()));
	if(r==count||std::feof(cfhd.native_handle()))
		return begin+r;
	throw std::system_error(errno,std::generic_category());
}

template<std::contiguous_iterator Iter>
inline void send(c_style_io_handle_unlocked& cfhd,Iter begin,Iter end)
{
	std::size_t const count(end-begin);
	if(
#ifdef _MSC_VER
	_fwrite_nolock
#else
	fwrite
#endif
	(std::to_address(begin),sizeof(*begin),count,cfhd.native_handle())<count)
		throw std::system_error(errno,std::generic_category());
}

template<bool err=false>
inline auto get(c_style_io_handle_unlocked& cfhd)
{
	auto ch(_fgetc_nolock(cfhd.native_handle()));
	if(ch==EOF)
	{
		if(std::feof(cfhd.native_handle()))
		{
			if constexpr(err)
				return std::pair<char,bool>{0,true};
			else
				throw eof();
		}
		throw std::system_error(errno,std::system_category());
	}
	if constexpr(err)
		return std::pair<char,bool>{std::char_traits<char>::to_char_type(ch),false};
	else
		return std::char_traits<char>::to_char_type(ch);
}

inline void put(c_style_io_handle_unlocked& cfhd,char ch)
{
	if(_fputc_nolock(ch,cfhd.native_handle())==EOF)
		throw std::system_error(errno,std::system_category());
}

inline void flush(c_style_io_handle_unlocked& cfhd)
{
	if(_fflush_nolock(cfhd.native_handle()))
		throw std::system_error(errno,std::system_category());
}

template<typename T,std::integral U>
inline void seek(c_style_io_handle_unlocked& cfhd,seek_type_t<T>,U i,seekdir s=seekdir::beg)
{
	if(_fseek_nolock(cfhd.native_handle(),seek_precondition<long,T,char>(i),static_cast<int>(s)))
		throw std::system_error(errno,std::system_category()); 
}

template<std::integral U>
inline void seek(c_style_io_handle_unlocked& cfhd,U i,seekdir s=seekdir::beg)
{
	seek(cfhd,seek_type<char>,i,s);
}

#ifdef __MINGW32__
inline char* oreserve(c_style_io_handle_unlocked& cfhd,std::size_t n)
{
	auto& h(*cfhd.native_handle());
	if((h._cnt-=n)<=0) [[unlikely]]
	{
		h._cnt+=n;
		return nullptr;
	}
	return h._ptr+=n;
}

inline void orelease(c_style_io_handle_unlocked& cfhd,std::size_t n)
{
	auto& h(*(_iobuf*)cfhd.native_handle());
	h._cnt+=n;
	h._ptr-=n;
}
#endif

template<typename... Args>
requires requires(std::FILE* fp,Args&& ...args)
{
	_fprintf_unlock(fp,std::forward<Args>(args)...);
}
inline auto fprintf(c_style_io_handle_unlocked& h,Args&& ...args)
{
	auto v(_fprintf_unlock(h.native_handle(),std::forward<Args>(args)...));
	if(v==EOF)
		throw std::system_error(errno,std::system_category());
	return v;
}

template<typename... Args>
requires requires(std::FILE* fp,Args&& ...args)
{
	_fscanf_nolock(fp,std::forward<Args>(args)...);
}
inline auto fscanf(c_style_io_handle_unlocked& h,Args&& ...args)
{
	auto v(_fscanf_nolock(h.native_handle(),std::forward<Args>(args)...));
	if(v==EOF)
		throw std::system_error(errno,std::system_category());
	return v;
}
#else
template<std::contiguous_iterator Iter>
inline Iter receive(c_style_io_handle_unlocked& cfhd,Iter begin,Iter end)
{
	std::size_t const count(end-begin);
	std::size_t const r(fread_unlock(std::to_address(begin),sizeof(*begin),count,cfhd.native_handle()));
	if(r==count||std::feof(cfhd.native_handle()))
		return begin+r;
	throw std::system_error(errno,std::generic_category());
}

template<std::contiguous_iterator Iter>
inline void send(c_style_io_handle_unlocked& cfhd,Iter begin,Iter end)
{
	std::size_t const count(end-begin);
	if(fwrite_unlocked(std::to_address(begin),sizeof(*begin),count,cfhd.native_handle())<count)
		throw std::system_error(errno,std::generic_category());
}

template<bool err=false>
inline auto get(c_style_io_handle_unlocked& cfhd)
{
	auto ch(fgetc_unlocked(cfhd.native_handle()));
	if(ch==EOF)
	{
		if(feof_unlocked(cfhd.native_handle()))
		{
			if constexpr(err)
				return std::pair<char,bool>{0,true};
			else
				throw fast_io::eof();
		}
		throw std::system_error(errno,std::system_category());
	}
	if constexpr(err)
		return std::pair<char,bool>{std::char_traits<char>::to_char_type(ch),false};
	else
		return std::char_traits<char>::to_char_type(ch);
}

inline void put(c_style_io_handle_unlocked& cfhd,char ch)
{
	if(fputc_unlocked(ch,cfhd.native_handle())==EOF)
		throw std::system_error(errno,std::system_category());
}

inline void flush(c_style_io_handle_unlocked& cfhd)
{
	if(fflush_unlocked(cfhd.native_handle()))
		throw std::system_error(errno,std::system_category());
}

template<typename T,std::integral U>
inline void seek(c_style_io_handle_unlocked& cfhd,seek_type_t<T>,U i,seekdir s=seekdir::beg)
{
	if(fseek(cfhd.native_handle(),seek_precondition<long,T,char>(i),static_cast<int>(s)))
		throw std::system_error(errno,std::system_category()); 
}

template<std::integral U>
inline void seek(c_style_io_handle_unlocked& cfhd,U i,seekdir s=seekdir::beg)
{
	seek(cfhd,seek_type<char>,i,s);
}
#ifdef __GNU_LIBRARY__
inline char* oreserve(c_style_io_handle_unlocked& cfhd,std::size_t n)
{
	auto& h(*cfhd.native_handle());
	if(h._IO_write_end<=h._IO_write_ptr+n)
		return nullptr;
	return h._IO_write_ptr+=n;
}

inline void orelease(c_style_io_handle_unlocked& cfhd,std::size_t n)
{
	cfhd.native_handle()->_IO_write_ptr-=n;
}
#endif
template<typename... Args>
requires requires(std::FILE* fp,Args&& ...args)
{
	fprintf_unlocked(fp,std::forward<Args>(args)...);
}
inline auto fprintf(c_style_io_handle_unlocked& h,Args&& ...args)
{
	auto v(fprintf_unlocked(h.native_handle(),std::forward<Args>(args)...));
	if(v==EOF)
		throw std::system_error(errno,std::system_category());
	return v;
}

template<typename... Args>
requires requires(std::FILE* fp,Args&& ...args)
{
	fscanf_unlocked(fp,std::forward<Args>(args)...);
}
inline auto fscanf(c_style_io_handle_unlocked& h,Args&& ...args)
{
	auto v(fscanf_unlocked(h.native_handle(),std::forward<Args>(args)...));
	if(v==EOF)
		throw std::system_error(errno,std::system_category());
	return v;
}

#endif

class c_style_io_lock_guard;

class c_style_io_handle
{
	std::FILE *fp;
protected:
	auto& protected_native_handle()
	{
		return fp;
	}
public:
	using lock_guard_type = c_style_io_lock_guard;
	c_style_io_handle(std::FILE* fpp):fp(fpp){}
	using char_type = char;
	using native_handle_type = std::FILE*;
	native_handle_type native_handle() const
	{
		return fp;
	}
};

inline auto fileno(c_style_io_handle& handle)
{
#if defined(__WINNT__) || defined(_MSC_VER)
	return _fileno(handle.native_handle());
#else
	return ::fileno(handle.native_handle());
#endif
}

inline auto mutex(c_style_io_handle& h)
{
	return h.native_handle();
}

inline auto unlocked_handle(c_style_io_handle& h)
{
	return c_style_io_handle_unlocked(h.native_handle());
}

class c_style_io_lock_guard
{
	std::FILE* const fp;
public:
	c_style_io_lock_guard(std::FILE* f):fp(f)
	{
#if defined(__WINNT__) || defined(_MSC_VER)
		_lock_file(fp);
#else
		flockfile(fp);
#endif
	}
	c_style_io_lock_guard(c_style_io_lock_guard const&) = delete;
	c_style_io_lock_guard& operator=(c_style_io_lock_guard const&) = delete;
	~c_style_io_lock_guard()
	{
#if defined(__WINNT__) || defined(_MSC_VER)
		_unlock_file(fp);
#else
		funlockfile(fp);
#endif
	}
};


template<std::contiguous_iterator Iter>
inline Iter receive(c_style_io_handle& cfhd,Iter begin,Iter end)
{
	std::size_t const count(end-begin);
	std::size_t const r(std::fread(std::to_address(begin),sizeof(*begin),count,cfhd.native_handle()));
	if(r==count||std::feof(cfhd.native_handle()))
		return begin+r;
	throw std::system_error(errno,std::generic_category());
}

template<std::contiguous_iterator Iter>
inline void send(c_style_io_handle& cfhd,Iter begin,Iter end)
{
	std::size_t const count(end-begin);
	if(std::fwrite(std::to_address(begin),sizeof(*begin),count,cfhd.native_handle())<count)
		throw std::system_error(errno,std::generic_category());
}

template<bool err=false>
inline auto get(c_style_io_handle& cfhd)
{
	auto ch(fgetc(cfhd.native_handle()));
	if(ch==EOF)
	{
		if(feof(cfhd.native_handle()))
		{
			if constexpr(err)
				return std::pair<char,bool>{0,true};
			else
				throw eof();
		}
		throw std::system_error(errno,std::system_category());
	}
	if constexpr(err)
		return std::pair<char,bool>{std::char_traits<char>::to_char_type(ch),false};
	else
		return std::char_traits<char>::to_char_type(ch);
}

inline void put(c_style_io_handle& cfhd,char ch)
{
	if(std::fputc(ch,cfhd.native_handle())==EOF)
		throw std::system_error(errno,std::system_category());
}

inline void flush(c_style_io_handle& cfhd)
{
	if(std::fflush(cfhd.native_handle()))
		throw std::system_error(errno,std::system_category());
}

template<typename T,std::integral U>
inline void seek(c_style_io_handle& cfhd,seek_type_t<T>,U i,seekdir s=seekdir::beg)
{
	if(std::fseek(cfhd.native_handle(),seek_precondition<long,T,char>(i),static_cast<int>(s)))
		throw std::system_error(errno,std::system_category()); 
}

template<typename... Args>
requires requires(std::FILE* fp,Args&& ...args)
{
	std::fprintf(fp,std::forward<Args>(args)...);
}
inline auto fprintf(c_style_io_handle& h,Args&& ...args)
{
	auto v(std::fprintf(h.native_handle(),std::forward<Args>(args)...));
	if(v==EOF)
		throw std::system_error(errno,std::system_category());
	return v;
}


template<typename... Args>
requires requires(std::FILE* fp,Args&& ...args)
{
	std::fscanf(fp,std::forward<Args>(args)...);
}
inline auto fscanf(c_style_io_handle& h,Args&& ...args)
{
	auto v(std::fscanf(h.native_handle(),std::forward<Args>(args)...));
	if(v==EOF)
		throw std::system_error(errno,std::system_category());
	return v;
}

template<std::integral U>
inline void seek(c_style_io_handle& cfhd,U i,seekdir s=seekdir::beg)
{
	seek(cfhd,seek_type<char>,i,s);
}

template<typename T>
class basic_c_style_file:public T
{
	using T::protected_native_handle;
	using T::native_handle;
	void close_impl() noexcept
	{
		if(native_handle())
			std::fclose(native_handle());
	}
public:
	using T::char_type;
	using T::native_handle_type;
	template<typename ...Args>
	basic_c_style_file(native_interface_t,Args&& ...args):T(std::fopen(std::forward<Args>(args)...))
	{
		if(native_handle()==nullptr)
			throw std::system_error(errno,std::generic_category());
	}
	basic_c_style_file(std::string_view name,std::string_view mode):basic_c_style_file(native_interface,name.data(),mode.data()){}
	basic_c_style_file(std::string_view file,open::mode const& m):basic_c_style_file(file,c_style(m))
	{
		if(with_ate(m))
			seek(*this,0,seekdir::end);
	}
	template<std::size_t om>
	basic_c_style_file(std::string_view name,open::interface_t<om>):basic_c_style_file(name,open::interface_t<om>::c_style)
	{
		if constexpr (with_ate(open::mode(om)))
			seek(*this,0,seekdir::end);
	}
	basic_c_style_file(basic_c_style_file const&)=delete;
	basic_c_style_file& operator=(basic_c_style_file const&)=delete;
	basic_c_style_file(basic_c_style_file&& b) noexcept : T(b.native_handle())
	{
		b.protected_native_handle() = nullptr;
	}
	basic_c_style_file& operator=(basic_c_style_file&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			protected_native_handle()=b.native_handle();
			b.protected_native_handle() = nullptr;
		}
		return *this;
	}
	~basic_c_style_file()
	{
		close_impl();
	}
};

using c_style_file = basic_c_style_file<c_style_io_handle>;
using c_style_file_unlocked = basic_c_style_file<c_style_io_handle_unlocked>;

}