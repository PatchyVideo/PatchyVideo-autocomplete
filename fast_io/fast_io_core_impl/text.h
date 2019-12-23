#pragma once

namespace fast_io
{
	
namespace details
{

template<typename>
struct text_view_interal_variable{};

template<character_input_stream T>
struct text_view_interal_variable<T>
{
	bool state=false;
	typename T::char_type internal_character{};
};

}

enum class operating_system
{
	win32,
	posix,
#if defined(__WINNT__) || defined(_MSC_VER)
	native=win32,
#else
	native=posix
#endif
};

template<typename T,bool sys=false>
requires character_input_stream<T>||character_output_stream<T>
class text_view
{
public:
	T ib;
	details::text_view_interal_variable<T> state;
public:
	using native_interface_t = T;
	using char_type = typename native_interface_t::char_type;
public:
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	constexpr text_view(Args&& ...args):ib(std::forward<Args>(args)...){}
	constexpr inline auto& native_handle()
	{
		return ib;
	}
};
/*
template<character_input_stream T,bool sys>
constexpr inline auto get(text_view<T,sys>& input)
{
	if(input.state.state)
	{
		input.state.state=false;
		return input.state.internal_character;
	}
	auto ch(get(input.native_handle()));
	if(ch==0xD)
	{
		auto internal(try_get(input.native_handle()));
		if(internal.second)
			return ch;
		if(internal.first==0xA)
			return internal.first;
		input.state.state=true;
		input.state.internal_character=internal.first;
	}
	return ch;
}
*/
template<bool err=false,character_input_stream T,bool sys>
constexpr inline auto get(text_view<T,sys>& input)
{
	if(input.state.state)
	{
		input.state.state=false;
		if constexpr(err)
			return std::pair<typename T::char_type,bool>{input.state.internal_character,false};
		else
			return input.state.internal_character;
	}
	auto ch(get<err>(input.native_handle()));
	if constexpr(err)
	{
		if(ch.second)
			return std::pair<typename T::char_type,bool>{0,true};
		if(ch.first==0xD)
		{
			auto internal(get<true>(input.native_handle()));
			if(internal.second)
				return ch;
			if(internal.first==0xA)
				return internal;
			input.state.state=true;
			input.state.internal_character=internal.first;
		}
	}
	else
	{
		if(ch==0xD)
		{
			auto internal(get<true>(input.native_handle()));
			if(internal.second)
				return ch;
			if(internal.first==0xA)
				return internal.first;
			input.state.state=true;
			input.state.internal_character=internal.first;
		}
	}
	return ch;
}

template<character_input_stream T,bool sys,std::contiguous_iterator Iter>
constexpr inline Iter receive(text_view<T,sys>& input,Iter b,Iter e)
{
	return define_receive_by_get(input,b,e);
}

template<character_output_stream T,bool sys>
constexpr inline void put(text_view<T,sys>& output,typename text_view<T,sys>::char_type ch)
{
	if constexpr((!sys)||operating_system::win32==operating_system::native)
		if(ch==0xA)
			put(output.ib,0xD);
	put(output.ib,ch);
}

template<character_output_stream T,bool sys,std::contiguous_iterator Iter>
constexpr inline void send(text_view<T,sys>& output,Iter b,Iter e)
{
	using char_type = T::char_type;
	if constexpr(sys&&operating_system::win32!=operating_system::native)
		send(output,b,e);
	else
	{
		auto pb(static_cast<char_type const*>(static_cast<void const*>(std::to_address(b))));
		auto last(pb);
		auto pi(pb),pe(pb+(e-b)*sizeof(*b)/sizeof(char_type));
		for(;pi!=pe;++pi)
			if(*pi==0xA)
			{
				send(output.ib,last,pi);
				put(output.ib,0xD);
				last=pi;
			}
		send(output.ib,last,pe);
	}
}

template<character_output_stream T,bool sys>
constexpr inline void flush(text_view<T,sys>& output)
{
	flush(output.ib);
}

template<stream T,bool sys>
inline constexpr void fill_nc(text_view<T,sys>& view,std::size_t count,typename T::char_type const& ch)
{
	if constexpr((!sys)||operating_system::win32==operating_system::native)
	{
		if(ch==0xA)
		{
			for(std::size_t i(0);i!=count;++i)
			{
				put(view.ib,0xD);
				put(view.ib,0xA);
			}
			return;
		}
	}
	fill_nc(view.ib,count,ch);
}

template<buffer_output_stream T,bool sys>
requires (sys&&operating_system::win32!=operating_system::native)
inline constexpr void oreserve(text_view<T,sys>& view,std::size_t size)
{
	oreserve(view.ib,size);
}

template<buffer_output_stream T,bool sys>
requires (sys&&operating_system::win32!=operating_system::native)
inline constexpr void orelease(text_view<T,sys>& view,std::size_t size)
{
	orelease(view.ib,size);
}


}