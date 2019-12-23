#pragma once

namespace fast_io
{
template<details::character_output_stream_impl T,std::forward_iterator Iter>
requires (std::same_as<typename std::iterator_traits<Iter>::value_type,typename T::char_type>)
inline constexpr void define_send_by_put(T& stm,Iter b,Iter e)
{
	for(auto i(b);i!=e;++i)
		put(stm,*i);
}

template<details::character_input_stream_impl T,std::forward_iterator Iter>
requires (std::same_as<typename std::iterator_traits<Iter>::value_type,typename T::char_type>)
inline constexpr Iter define_receive_by_get(T& stm,Iter b,Iter e)
{
	for(auto i(b);i!=e;++i)
	{
		auto [ch,err](get<true>(stm));
		if(err) [[unlikely]]
			return i;
		*i=ch;
	}
	return e;
}

template<fast_io::output_stream output,std::ranges::random_access_range range>
inline void send(output& out,range const& rg)
{
	send(out,cbegin(rg),cend(rg));
}

template<fast_io::input_stream input,std::ranges::random_access_range range>
inline void receive(input& in,range& rg)
{
	receive(in,begin(rg),end(rg));
}


}