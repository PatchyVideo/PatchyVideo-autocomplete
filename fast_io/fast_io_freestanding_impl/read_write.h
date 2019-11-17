#pragma once

namespace fast_io
{

template<character_input_stream input,Trivial_copyable T>
inline constexpr void read(input& in,T& v)
{
	auto address(std::addressof(v));
	if(reads(in,address,address+1)!=(address+1))
		throw std::runtime_error("cannot read data from input_stream&");
}

template<output_stream output,Trivial_copyable T>
inline constexpr void write_define(output& out,T const& v)
{
	auto address(std::addressof(v));
	writes(out,address,address+1);
}

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
	throw std::runtime_error("size is out of std::size_t range");
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

template<character_input_stream input,Dynamic_size_container D>
inline constexpr void read_define(input& in,D& v)
{
	v.resize(read_size(in));
	for(auto & e : v)
		read(in,e);
}

template<character_input_stream input,Contiguous_trivial_dynamic_size_container D>
inline constexpr void read_define(input& in,D& v)
{
	v.resize(read_size(in));
	if(reads(in,v.begin(),v.end())!=v.end())
		throw std::runtime_error("read contiguous trivial containers error");
}

template<character_output_stream output,Contiguous_trivial_dynamic_size_container D>
inline constexpr void write_define(output& out,D const& v)
{
	write_size(out,v.size());
	writes(out,v.begin(),v.end());
}

template<character_output_stream output,Dynamic_size_container D>
inline constexpr void write_define(output& out,D const& v)
{
	write_size(out,v.size());
	for(auto const& e : v)
		write(out,e);
}

template<character_output_stream output,Contiguous_fixed_size_none_trivial_copyable_container D>
inline constexpr void write_define(output& out,D const& v)
{
	for(auto const& e : v)
		write(out,e);
}

}