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
inline constexpr std::size_t skip_line(input& in)
{
	std::size_t skipped(0);
	for(decltype(get<true>(in)) ch;!(ch=get<true>(in)).second&&ch.first!=0xA;++skipped);
	return skipped;
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
	auto ch(get<true>(in));
	for(;details::isspace(ch.first);ch=get<true>(in));
	return ch;
}

template<character_input_stream input,std::integral T>
requires std::same_as<T,bool>
inline constexpr void scan_define(input& in, T& b)
{
	auto value(eat_space_get(in));
	if(value==0x30)
		b=0;
	else
		b=1;
}

template<character_output_stream output,std::integral T>
requires std::same_as<T,bool>
inline constexpr void print_define(output& out, T const& b)
{
	put(out,b+0x30);
}

template<output_stream output>
inline constexpr void print_define(output& out,std::basic_string_view<typename output::char_type> str)
{
	send(out,str.data(),str.data()+str.size());
}

template<output_stream output>
requires (std::same_as<typename output::char_type,char>)
inline constexpr void print_define(output& out,std::basic_string_view<char8_t> str)
{
	send(out,str.data(),str.data()+str.size());
}

inline namespace print_scan_details
{
template<input_stream input,typename ...Args>
requires(scanable<input,Args>&&...)
inline constexpr void normal_scan(input &in,Args&& ...args)
{
	(scan_define(in,std::forward<Args>(args)),...);
}

template<output_stream output,typename ...Args>
requires(printable<output,Args>&&...)
inline constexpr void normal_print(output &out,Args&& ...args)
{
	(print_define(out,std::forward<Args>(args)),...);
}

template<output_stream output,typename ...Args>
requires((sizeof...(Args)==1&&(printlnable<output,Args>&&...))||(character_output_stream<output>&&(printable<output,Args>&&...)))
inline constexpr void normal_println(output &out,Args&& ...args)
{
	if constexpr((sizeof...(Args)==1)&&(printlnable<output,Args>&&...))
	{
		(println_define(out,std::forward<Args>(args)),...);
	}
	else
	{
		(print_define(out,std::forward<Args>(args)),...);
		put(out,0xA);
	}
}

template<input_stream input,typename ...Args>
requires(readable<input,Args>&&...)
inline constexpr void normal_read(input &in,Args&& ...args)
{
	(read_define(in,std::forward<Args>(args)),...);
}

template<output_stream output,typename ...Args>
requires(writeable<output,Args>&&...)
inline constexpr void normal_write(output &out,Args&& ...args)
{
	(write_define(out,std::forward<Args>(args)),...);
}

}

template<input_stream input,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void scan(input &in,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_input_stream<input>)
	{
		typename input::lock_guard_type lg{mutex(in)};
		decltype(auto) uh(unlocked_handle(in));
		scan(uh,std::forward<Args>(args)...);
	}
	else if constexpr(true)
		normal_scan(in,std::forward<Args>(args)...);
}

template<input_stream input,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void read(input &in,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_input_stream<input>)
	{
		typename input::lock_guard_type lg{mutex(in)};
		decltype(auto) uh(unlocked_handle(in));
		read(uh,std::forward<Args>(args)...);
	}
	else if constexpr(true)
		normal_read(in,std::forward<Args>(args)...);
}

template<output_stream output,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void print(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		print(uh,std::forward<Args>(args)...);
	}
	else if constexpr((printable<output,Args>&&...)&&(sizeof...(Args)==1||buffer_output_stream<output>))
		normal_print(out,std::forward<Args>(args)...);
	else if constexpr(true)
		buffer_print(out,std::forward<Args>(args)...);
}

template<output_stream output,typename ...Args>
inline constexpr void println(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		println(uh,std::forward<Args>(args)...);
	}
	else if constexpr((sizeof...(Args)==1&&(printlnable<output,Args>&&...))||
	((printable<output,Args>&&...)&&buffer_output_stream<output>&&character_output_stream<output>))
		normal_println(out,std::forward<Args>(args)...);
	else if constexpr(true)
		buffer_println(out,std::forward<Args>(args)...);
}

template<output_stream output,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void write(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		write(uh,std::forward<Args>(args)...);
	}
	else if constexpr((writeable<output,Args>&&...)&&(sizeof...(Args)==1||buffer_output_stream<output>))
		normal_write(out,std::forward<Args>(args)...);
	else if constexpr(true)
		buffer_write(out,std::forward<Args>(args)...);
}


template<output_stream output,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void print_flush(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		print_flush(uh,std::forward<Args>(args)...);
	}
	else
	{
		if constexpr((printable<output,Args>&&...)&&(sizeof...(Args)==1||buffer_output_stream<output>))
			normal_print(out,std::forward<Args>(args)...);
		else if constexpr(true)
			buffer_print(out,std::forward<Args>(args)...);
		flush(out);
	}
}

template<output_stream output,typename ...Args>
inline constexpr void println_flush(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		println_flush(out.native_handle(),std::forward<Args>(args)...);
	}
	else
	{
		if constexpr((sizeof...(Args)==1&&(printlnable<output,Args>&&...))||
			((printable<output,Args>&&...)&&buffer_output_stream<output>&&character_output_stream<output>))
			normal_println(out,std::forward<Args>(args)...);
		else if constexpr(true)
			buffer_println(out,std::forward<Args>(args)...);
		flush(out);
	}
}

template<output_stream output,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void write_flush(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		write_flush(out.native_handle(),std::forward<Args>(args)...);
	}
	else
	{
		if constexpr((writeable<output,Args>&&...)&&(sizeof...(Args)==1||buffer_output_stream<output>))
			normal_write(out,std::forward<Args>(args)...);
		else if constexpr(true)
			buffer_write(out,std::forward<Args>(args)...);
		flush(out);
	}
}


inline namespace print_scan_details
{
template<output_stream os,typename ch_type,typename ...Args>
inline void fprint_impl(os &out,std::basic_string_view<ch_type> format)
{
	std::size_t percent_pos;
	for(;(percent_pos=format.find(0X25))!=std::string_view::npos&&percent_pos+1!=format.size()&&format[percent_pos+1]==0X25;format.remove_prefix(percent_pos+2))
		send(out,format.cbegin(),format.cbegin()+percent_pos+1);
	if(percent_pos!=std::string_view::npos)
		std::terminate();
	send(out,format.cbegin(),format.cend());
}

template<output_stream os,typename ch_type,typename T,typename ...Args>
inline void fprint_impl(os &out,std::basic_string_view<ch_type> format,T&& cr,Args&& ...args)
{
	std::size_t percent_pos;
	for(;(percent_pos=format.find(0X25))!=std::string_view::npos&&percent_pos+1!=format.size()&&format[percent_pos+1]==0X25;format.remove_prefix(percent_pos+2))
		send(out,format.cbegin(),format.cbegin()+percent_pos+1);
	if(percent_pos==std::string_view::npos)
	{
		send(out,format.cbegin(),format.cend());
		return;
	}
	else
	{
		send(out,format.cbegin(),format.cbegin()+percent_pos);
		format.remove_prefix(percent_pos+1);
	}
	print(out,std::forward<T>(cr));
	fprint_impl(out,format,std::forward<Args>(args)...);
}

template<output_stream output,typename ch_type,typename ...Args>
requires((std::same_as<typename output::char_type,ch_type>||std::same_as<char8_t,ch_type>)&&(printable<output,Args>&&...))
inline constexpr void normal_fprint(output &out,std::basic_string_view<ch_type> mv,Args&& ...args)
{
	fprint_impl(out,mv,std::forward<Args>(args)...);
}

}

template<output_stream output,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void fprint(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		fprint(uh,std::forward<Args>(args)...);
	}
	else if constexpr((printable<output,Args>&&...)&&(sizeof...(Args)==1||(buffer_output_stream<output>&&character_output_stream<output>)))
		normal_fprint(out,std::forward<Args>(args)...);
	else if constexpr(true)
		buffer_fprint(out,std::forward<Args>(args)...);
}

template<output_stream output,typename ...Args>
requires (sizeof...(Args)!=0)
inline constexpr void fprint_flush(output &out,Args&& ...args)
{
	using namespace print_scan_details;
	if constexpr(mutex_output_stream<output>)
	{
		typename output::lock_guard_type lg{mutex(out)};
		decltype(auto) uh(unlocked_handle(out));
		fprint_flush(uh,std::forward<Args>(args)...);
	}
	else
	{
		if constexpr((printable<output,Args>&&...)&&(sizeof...(Args)==1||buffer_output_stream<output>))
			normal_fprint(out,std::forward<Args>(args)...);
		else if constexpr(true)
			buffer_fprint(out,std::forward<Args>(args)...);
		flush(out);
	}
}


}