#pragma once

namespace fast_io
{


template<character_input_stream input>
inline constexpr std::size_t read_size(input& in)
{
	auto constexpr bytes(sizeof(get(in))*8);
	auto constexpr lshift(bytes-1);
	auto constexpr limit(static_cast<std::size_t>(1)<<lshift);
	auto constexpr limitm1(limit-1);
	for(std::size_t temp(0),lsf(0);lsf<sizeof(std::size_t)*8;lsf+=lshift)
	{
		std::make_unsigned_t<decltype(get(in))> ch(get(in));
		temp|=(ch&limitm1)<<lsf;
		if(ch<limit)
			return temp;
	}
	throw std::runtime_error(reinterpret_cast<char const*>(u8"size is out of std::size_t range"));
}

template<character_output_stream output>
inline constexpr void write_size(output& out,std::size_t size)
{
	using ch_type = typename output::char_type;
	auto constexpr bytes(sizeof(typename output::char_type)*8);
	auto constexpr lshift(bytes-1);
	auto constexpr limit(static_cast<std::size_t>(1)<<lshift);
	auto constexpr limitm1(limit-1);
	for(;limitm1<size;size>>=lshift)
		put(out,static_cast<ch_type>((size&limitm1)|limit));
	put(out,static_cast<ch_type>(size));
}

namespace details
{
template<typename T,std::size_t N>
inline constexpr bool detect_std_array(std::array<T,N> const&) {return true;}

template<typename T>
inline constexpr bool detect_std_array(T const&) {return false;}
}

template<output_stream output,typename T>
requires (std::is_trivially_copyable_v<T>||std::ranges::sized_range<T>)
inline constexpr void write_define(output& out,T const& v)
{
	if constexpr(std::is_trivially_copyable_v<T>)
	{
		if constexpr(std::is_bounded_array_v<T>)
			write_size(out,std::size(v));
		auto address(std::addressof(v));
		send(out,address,address+1);
	}
	else
	{
		if constexpr(!details::detect_std_array(v))
			write_size(out,std::size(v));
		for(auto const& e : v)
			write(out,e);
	}
}
template<input_stream input,typename T>
requires (std::is_trivially_copyable_v<T>||std::ranges::sized_range<T>)
inline constexpr void read_define(input& in,T& v)
{
	if constexpr(std::is_trivially_copyable_v<T>)
	{
		auto address(std::addressof(v));
		receive(in,address,address+1);
	}
	else
	{
		if constexpr(!details::detect_std_array(v)&&!std::is_bounded_array_v<T>)
			v.resize(read_size(in));
		for(auto& e : v)
			read(in,e);
	}
}


}