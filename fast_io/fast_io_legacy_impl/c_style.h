#pragma once
#include<cstdio>

namespace fast_io
{

class c_style_io_handle
{
	std::FILE *fp;
protected:
	auto& protected_native_handle()
	{
		return fp;
	}
public:
	c_style_io_handle(std::FILE* fpp):fp(fpp){}
	using char_type = char;
	using native_handle_type = std::FILE*;
	native_handle_type native_handle() const
	{
		return fp;
	}
	bool eof() const
	{
		return std::feof(fp);
	}
	template<std::contiguous_iterator Iter>
	Iter reads(Iter begin,Iter end)
	{
		std::size_t const count(end-begin);
		std::size_t const r(std::fread(std::to_address(begin),sizeof(*begin),count,fp));
		if(r==count||std::feof(fp))
			return begin+r;
		throw std::system_error(errno,std::generic_category());
	}

	template<std::contiguous_iterator Iter>
	void writes(Iter begin,Iter end)
	{
		std::size_t const count(end-begin);
		if(std::fwrite(std::to_address(begin),sizeof(*begin),count,fp)<count)
			throw std::system_error(errno,std::generic_category());
	}
	std::pair<char_type,bool> try_get()
	{
		auto ch(fgetc(fp));
		if(ch==EOF)
		{
			if(feof(fp))
				return {0,true};
			throw std::system_error(errno,std::system_category());
		}
		return {ch,false};
	}
	char_type get()
	{
		auto ch(fgetc(fp));
		if(ch==EOF)
		{
			if(feof(fp))
				throw eof();
			throw std::system_error(errno,std::system_category());
		}
		return ch;
	}
	void put(char_type ch)
	{
		if(fputc(ch,fp)==EOF)
			throw std::system_error(errno,std::system_category());
	}
	void flush()
	{
		if(std::fflush(fp))
			throw std::system_error(errno,std::system_category());
	}
	template<typename T,std::integral U>
	void seek(seek_type_t<T>,U i,seekdir s=seekdir::beg)
	{
		if(fseek(fp,seek_precondition<long,T,char_type>(i),static_cast<int>(s)))
			throw std::system_error(errno,std::system_category()); 
	}
	template<std::integral U>
	void seek(U i,seekdir s=seekdir::beg)
	{
		seek(seek_type<char_type>,i,s);
	}
};

class c_style_file:public c_style_io_handle
{
	void close_impl() noexcept
	{
		if(native_handle())
			std::fclose(native_handle());
	}
public:
	using char_type = c_style_io_handle::char_type;
	using native_handle_type = c_style_io_handle::native_handle_type;
	template<typename ...Args>
	c_style_file(native_interface_t,Args&& ...args):c_style_io_handle(std::fopen(std::forward<Args>(args)...))
	{
		if(native_handle()==nullptr)
			throw std::system_error(errno,std::generic_category());
	}
	c_style_file(std::string_view name,std::string_view mode):c_style_file(native_interface,name.data(),mode.data()){}
	c_style_file(std::string_view file,open::mode const& m):c_style_file(file,c_style(m))
	{
		if(with_ate(m))
			seek(0,seekdir::end);
	}
	template<std::size_t om>
	c_style_file(std::string_view name,open::interface_t<om>):c_style_file(name,open::interface_t<om>::c_style)
	{
		if constexpr (with_ate(open::mode(om)))
			seek(0,seekdir::end);
	}
	c_style_file(c_style_file const&)=delete;
	c_style_file& operator=(c_style_file const&)=delete;
	c_style_file(c_style_file&& b) noexcept : c_style_io_handle(b.native_handle())
	{
		b.protected_native_handle() = nullptr;
	}
	c_style_file& operator=(c_style_file&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			protected_native_handle()=b.native_handle();
			b.protected_native_handle() = nullptr;
		}
		return *this;
	}
	~c_style_file()
	{
		close_impl();
	}
};

}