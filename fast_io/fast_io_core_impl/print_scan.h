#pragma once

namespace fast_io
{

namespace details
{

template<std::integral T>
inline constexpr bool isspace(T ch)
{
	if(ch==0x20)
		return true;
	std::make_unsigned_t<T> e(ch);
	e-=9;
	return e<5;
}

}

template<character_input_stream input>
inline constexpr auto eat_space_get(input& in)
{
	decltype(get(in)) ch(get(in));
	for(;details::isspace(ch);ch=get(in));
	return ch;
}

template<character_input_stream input>
inline constexpr auto try_eat_space_get(input& in)
{
	auto ch(try_get(in));
	for(;details::isspace(ch.first);ch=try_get(in));
	return ch;
}

template<character_input_stream input,std::integral T>
requires std::same_as<T,bool>
inline constexpr void scan_define(input& in, T& b)
{
	auto value(eat_space_get(in));
	if(value=='0')
		b=0;
	else
		b=1;
}

template<character_output_stream output,std::integral T>
requires std::same_as<T,bool>
inline constexpr void print_define(output& out, T const& b)
{
	put(out,b+'0');
}

template<output_stream output>
inline constexpr void print_define(output& out,std::basic_string_view<typename output::char_type> str)
{
	writes(out,str.data(),str.data()+str.size());
}

namespace details
{
template<output_stream os,typename ...Args>
inline void fprint_impl(os &out,std::basic_string_view<typename os::char_type> format)
{
	std::size_t percent_pos;
	for(;(percent_pos=format.find('%'))!=std::string_view::npos&&percent_pos+1!=format.size()&&format[percent_pos+1]=='%';format.remove_prefix(percent_pos+2))
		writes(out,format.cbegin(),format.cbegin()+percent_pos+1);
#ifdef __EXCEPTIONS
	if(percent_pos!=std::string_view::npos)
		throw std::runtime_error("fprint() format error");
#endif
	writes(out,format.cbegin(),format.cend());
}

template<output_stream os,typename T,typename ...Args>
inline void fprint_impl(os &out,std::basic_string_view<typename os::char_type> format,T&& cr,Args&& ...args)
{
	std::size_t percent_pos;
	for(;(percent_pos=format.find('%'))!=std::string_view::npos&&percent_pos+1!=format.size()&&format[percent_pos+1]=='%';format.remove_prefix(percent_pos+2))
		writes(out,format.cbegin(),format.cbegin()+percent_pos+1);
	if(percent_pos==std::string_view::npos)
	{
		writes(out,format.cbegin(),format.cend());
		return;
	}
	else
	{
		writes(out,format.cbegin(),format.cbegin()+percent_pos);
		format.remove_prefix(percent_pos+1);
	}
	print(out,std::forward<T>(cr));
	fprint_impl(out,format,std::forward<Args>(args)...);
}
}


template<input_stream input,typename ...Args>
requires(scanable<input,Args>&&...)
inline constexpr void scan(input &in,Args&& ...args)
{
	(scan_define(in,std::forward<Args>(args)),...);
}

template<output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void print(output &out,Args&& ...args)
{
	(print_define(out,std::forward<Args>(args)),...);
}

template<character_output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void println(output &out,Args&& ...args)
{
	(print_define(out,std::forward<Args>(args)),...);
	put(out,'\n');
}

template<output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void fprint(output &out,std::basic_string_view<typename output::char_type> format)
{
	details::fprint_impl(out,format);
}

template<output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void fprint(output &out,std::basic_string_view<typename output::char_type> format,Args&& ...args)
{
	details::fprint_impl(out,format,std::forward<Args>(args)...);
}

template<output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void print_flush(output &out,Args&& ...args)
{
	(print_define(out,std::forward<Args>(args)),...);
	flush(out);
}

template<character_output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void println_flush(output &out,Args&& ...args)
{
	(print_define(out,std::forward<Args>(args)),...);
	put(out,'\n');
	flush(out);
}

template<output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void fprint_flush(output &out,std::basic_string_view<typename output::char_type> format,Args&& ...args)
{
	details::fprint_impl(out,format,std::forward<Args>(args)...);
	flush(out);
}


template<input_stream input,typename ...Args>
requires(readable<input,Args>&&...)
inline constexpr void read(input &in,Args&& ...args)
{
	(read_define(in,std::forward<Args>(args)),...);
}

template<output_stream output,typename ...Args>
requires(writeable<output,Args>&&...)
inline constexpr void write(output &out,Args&& ...args)
{
	(write_define(out,std::forward<Args>(args)),...);
}

template<output_stream output,typename ...Args>
requires(writeable<output,Args>&&...)
inline constexpr void write_flush(output &out,Args&& ...args)
{
	(write_define(out,std::forward<Args>(args)),...);
	flush(out);
}

}