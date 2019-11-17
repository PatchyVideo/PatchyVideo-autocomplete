#pragma once

namespace fast_io
{

template<typename output,typename T>
concept weak_printable=output_stream<output>&&(!printable<output,T>)&&requires(basic_ostring<std::basic_string<typename output::char_type>>& ostr,T&& t)
{
	print_define(ostr,std::forward<T>(t));
};

template<typename output,typename T>
concept weak_writeable=output_stream<output>&&(!writeable<output,T>)&&requires(basic_ostring<std::basic_string<typename output::char_type>>& ostr,T&& t)
{
	write_define(ostr,std::forward<T>(t));
};

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void print(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(print_define(ostr,std::forward<Args>(args)),...);
	writes(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires ((!character_output_stream<output>)||(weak_printable<output,Args>||...))
inline constexpr void println(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(print_define(ostr,std::forward<Args>(args)),...);
	put(ostr,'\n');
	writes(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void fprint(output &out,std::basic_string_view<typename output::char_type> format,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	details::fprint_impl(ostr,format,std::forward<Args>(args)...);
	writes(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires(weak_writeable<output,Args>||...)
inline constexpr void write(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(write_define(ostr,std::forward<Args>(args)),...);
	writes(out,ostr.str().cbegin(),ostr.str().cend());
}

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void print_flush(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(print_define(ostr,std::forward<Args>(args)),...);
	writes(out,ostr.str().cbegin(),ostr.str().cend());
	flush(out);
}

template<output_stream output,typename ...Args>
requires ((!character_output_stream<output>)||(weak_printable<output,Args>||...))
inline constexpr void println_flush(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(print_define(ostr,std::forward<Args>(args)),...);
	put(ostr,'\n');
	writes(out,ostr.str().cbegin(),ostr.str().cend());
	flush(out);
}

template<output_stream output,typename ...Args>
requires(weak_printable<output,Args>||...)
inline constexpr void fprint_flush(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	details::fprint_impl(ostr,format,std::forward<Args>(args)...);
	put(ostr,'\n');
	writes(out,ostr.str().cbegin(),ostr.str().cend());
	flush(out);
}

template<output_stream output,typename ...Args>
requires(weak_writeable<output,Args>||...)
inline constexpr void write_flush(output &out,Args&& ...args)
{
	basic_ostring<std::basic_string<typename output::char_type>> ostr;
	(write_define(ostr,std::forward<Args>(args)),...);
	writes(out,ostr.str().cbegin(),ostr.str().cend());
	flush(out);
}

}