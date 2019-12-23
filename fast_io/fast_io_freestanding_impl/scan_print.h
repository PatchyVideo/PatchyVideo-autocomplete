#pragma once

namespace fast_io
{
inline namespace print_scan_details
{
template<typename output,typename T>
concept weak_printable=output_stream<output>&&requires(basic_ostring<std::basic_string<typename output::char_type>>& ostr,T const& t)
{
	print_define(ostr,t);
};

template<typename output,typename T>
concept weak_writeable=output_stream<output>&&requires(basic_ostring<std::basic_string<typename output::char_type>>& ostr,T const& t)
{
	write_define(ostr,t);
};

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void buffer_print(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(print_define(ostr,std::forward<Args>(args)),...);
	send(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires (weak_printable<output,Args>||...)
inline constexpr void buffer_println(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(print_define(ostr,std::forward<Args>(args)),...);
	put(ostr,0xA);
	send(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void buffer_fprint(output &out,std::basic_string_view<typename output::char_type> format,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	print_scan_details::fprint_impl(ostr,format,std::forward<Args>(args)...);
	send(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void buffer_fprint(output &out,std::basic_string_view<char8_t> format,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	print_scan_details::fprint_impl(ostr,format,std::forward<Args>(args)...);
	send(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires(weak_writeable<output,Args>||...)
inline constexpr void buffer_write(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(write_define(ostr,std::forward<Args>(args)),...);
	send(out,ostr.str().cbegin(),ostr.str().cend());
}
}
}