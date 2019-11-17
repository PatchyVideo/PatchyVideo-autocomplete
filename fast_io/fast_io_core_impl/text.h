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
	typename T::char_type internal_character = 0;
};

}

template<typename T>
requires character_input_stream<T>||character_output_stream<T>
class text_view
{
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
	constexpr inline char_type mmget() requires character_input_stream<T>
	{
		if(state.state)
		{
			state.state=false;
			return state.internal_character;
		}
		auto ch(get(ib));
		if(ch=='\r')
		{
			auto internal(try_get(ib));
			if(internal.second)
				return ch;
			if(internal.first=='\n')
				return '\n';
			state.state=true;
			state.internal_character=internal.first;
		}
		return ch;
	}
	constexpr inline std::pair<char_type,bool> mmtry_get() requires character_input_stream<T>
	{
		if(state.state)
		{
			state.state=false;
			return {state.internal_character,false};
		}
		auto ch(try_get(ib));
		if(ch.second)
			return {0,true};
		if(ch.first=='\r')
		{
			auto internal(try_get(ib));
			if(internal.second)
				return ch;
			if(internal.first=='\n')
				return internal;
			state.state=true;
			state.internal_character=internal.first;
		}
		return ch;
	}
	template<std::contiguous_iterator Iter>
	constexpr inline Iter mmreads(Iter b,Iter e)
		requires character_input_stream<T>
	{
		auto pb(static_cast<char_type*>(static_cast<void*>(std::to_address(b))));
		auto pe(pb+(e-b)*sizeof(*b)/sizeof(char_type));
		auto pi(pb);
		for(;pi!=pe;++pi)
			*pi=mmget();
		return b+(pi-pb)*sizeof(char_type)/sizeof(*b);
	}
	constexpr inline void mmput(char_type ch)	requires character_output_stream<T>
	{
		if(ch=='\n')
			put(ib,'\r');
		put(ib,ch);
	}
	template<std::contiguous_iterator Iter>
	constexpr inline void mmwrites(Iter b,Iter e)
		requires character_output_stream<T>
	{
		auto pb(static_cast<char_type const*>(static_cast<void const*>(std::to_address(b))));
		auto last(pb);
		auto pi(pb),pe(pb+(e-b)*sizeof(*b)/sizeof(char_type));
		for(;pi!=pe;++pi)
			if(*pi=='\n')
			{
				if(last!=pi)
					writes(ib,last,pi-1);
				put(ib,'\r');
				put(ib,'\n');
				last=pi+1;
			}
		writes(ib,last,pe);
	}
};

template<stream srm>
text_view(srm&&) -> text_view<srm>;

template<character_input_stream T>
constexpr inline auto get(text_view<T>& input)
{
	return input.mmget();
}

template<character_input_stream T>
constexpr inline auto try_get(text_view<T>& input)
{
	return input.mmtry_get();
}

template<character_input_stream T,std::contiguous_iterator Iter>
constexpr inline Iter reads(text_view<T>& input,Iter b,Iter e)
{
	return input.mmreads(b,e);
}

template<character_output_stream T>
constexpr inline void put(text_view<T>& output,typename text_view<T>::char_type ch)
{
	output.mmput(ch);
}

template<character_output_stream T,std::contiguous_iterator Iter>
constexpr inline void writes(text_view<T>& output,Iter b,Iter e)
{
	output.mmwrites(b,e);
}

template<character_output_stream T>
constexpr inline void flush(text_view<T>& output)
{
	flush(output.native_handle());
}

template<stream T>
inline constexpr void fill_nc(text_view<T>& view,std::size_t count,typename T::char_type const& ch)
{
	if(ch=='\n')
	{
		for(std::size_t i(0);i!=count;++i)
		{
			put(view.native_handle(),'\r');
			put(view.native_handle(),'\n');
		}
		return;
	}
	fill_nc(view.native_handle(),count,ch);
}


}