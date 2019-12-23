#pragma once

namespace fast_io
{
namespace manip
{
template<typename T>
struct char_view
{
	T& reference;
};

template<std::integral T>
struct unsigned_view
{
	T& reference;
};

template<std::integral T>
struct signed_view
{
	T& reference;
};

template<std::size_t w,bool left,char char_type,typename T>
struct width
{
	T& reference;
};

template<std::size_t precision,typename T>
struct fixed
{
	T& reference;
};

template<std::size_t precision,bool uppercase_e,typename T>
struct scientific
{
	T& reference;
};

template<typename T>
struct fixed_shortest
{
	T& reference;
};

template<bool uppercase_e,typename T>
struct scientific_shortest
{
	T& reference;
};

template<std::size_t precision,bool uppercase_e,typename T>
struct shortest
{
	T& reference;
};

template<bool uppercase_e,typename T>
struct shortest_shortest
{
	T& reference;
};

}
template<typename T>
requires (std::floating_point<T>||std::integral<T>)
inline constexpr manip::char_view<T> char_view(T& ch)
{
	return {ch};
}
template<typename T>
requires (std::floating_point<T>||std::integral<T>)
inline constexpr manip::char_view<T const> char_view(T const& ch)
{
	return {ch};
}

template<std::integral T>
inline constexpr decltype(auto) unsigned_view(T& value)
{
	return reinterpret_cast<std::make_unsigned_t<T>&>(value);
}

template<std::integral T>
inline constexpr decltype(auto) signed_view(T& value)
{
	return reinterpret_cast<std::make_signed_t<T>&>(value);
}

template<std::integral T>
inline constexpr decltype(auto) unsigned_view(T const& value)
{
	return reinterpret_cast<std::make_unsigned_t<T const>&>(value);
}

template<std::integral T>
inline constexpr decltype(auto) floating_view(T const& value)
{
	return static_cast<long double>(value);
}

template<std::floating_point F>
inline constexpr decltype(auto) floating_view(F const& p)
{
	return p;
}

template<std::floating_point F>
inline constexpr decltype(auto) unsigned_view(F const& f)
{
	return static_cast<std::uintmax_t>(f);
}

template<std::floating_point F>
inline constexpr decltype(auto) signed_view(F const& f)
{
	return static_cast<std::intmax_t>(f);
}

template<std::integral T>
inline constexpr decltype(auto) signed_view(T const& value)
{
	return reinterpret_cast<std::make_signed_t<T const>&>(value);
}

template<typename T>
inline constexpr std::size_t unsigned_view(T * const pointer)
{
	return bit_cast<std::size_t>(pointer);
}

template<std::size_t precision,typename T>
inline constexpr manip::fixed<precision,T const> fixed(T const &f){return {f};}

template<std::size_t precision,typename T>
inline constexpr manip::scientific<precision,false,T const> scientific(T const &f){return {f};}
template<std::size_t precision,typename T>
inline constexpr manip::scientific<precision,true,T const> scientific_upper(T const &f){return {f};}

template<typename T>
inline constexpr manip::scientific_shortest<false,T const> scientific(T const &f){return {f};}
template<typename T>
inline constexpr manip::scientific_shortest<true,T const> scientific_upper(T const &f){return {f};}

template<typename T>
inline constexpr manip::fixed_shortest<T const> fixed(T const &f){return {f};}


template<typename T>
inline constexpr manip::shortest_shortest<false,T const> shortest(T const &f){return {f};}
template<typename T>
inline constexpr manip::shortest_shortest<true,T const> shortest_upper(T const &f){return {f};}

template<character_input_stream input,std::integral T>
inline void scan_define(input& in,manip::char_view<T> a)
{
	a.reference = get(in);
}

template<character_output_stream output,std::integral T>
inline void print_define(output& out,manip::char_view<T> a)
{
	put(out,static_cast<typename output::char_type>(a.reference));
}

template<character_input_stream input,std::floating_point T>
inline void scan_define(input& in,manip::char_view<T> a)
{
	a.reference = get(in);
}

template<character_output_stream output,std::floating_point T>
inline void print_define(output& out,manip::char_view<T> a)
{
	put(out,static_cast<typename output::char_type>(a.reference));
}

template<std::size_t indent_w,bool left=false,char fill_ch=0x20,typename T>
inline constexpr manip::width<indent_w,left,fill_ch,T const> width(T const& t)
{
	return {t};
}

}