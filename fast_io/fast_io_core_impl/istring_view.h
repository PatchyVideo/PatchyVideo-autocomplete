#pragma once


namespace fast_io
{

template< typename T>
class basic_istring_view
{
	T s;
public:
	using char_type = typename T::value_type;
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	constexpr basic_istring_view(Args&& ...args):s(std::forward<Args>(args)...){}
	constexpr auto& str()
	{
		return s;
	}
	constexpr auto empty() const {return s.empty();}
};
template<typename T>
[[nodiscard]] inline constexpr std::size_t isize(basic_istring_view<T>& isv)
{
	return isv.str().size();
}

template<typename T>
[[nodiscard]] inline constexpr auto ireserve(basic_istring_view<T>& isv,std::size_t size)
{
	isv.str().remove_prefix(size);
	return isv.str().begin();
}

template<typename T>
inline constexpr void irelease(basic_istring_view<T>& isv,std::size_t size)
{
	isv.str()={isv.str().data()-size,isv.str().size()+size};
}

template<output_stream output, typename T>
inline constexpr void idump(output& out,basic_istring_view<T>& isv)
{
	writes(out,isv.str().data(),isv.str().data()+isv.str().size());
	isv.str()={};
}

template<typename T,std::contiguous_iterator Iter>
inline constexpr Iter reads(basic_istring_view<T>& istrvw,Iter begin,Iter end)
{
	auto pb(static_cast<typename T::value_type*>(static_cast<void*>(std::to_address(begin))));
	auto pe(static_cast<typename T::value_type*>(static_cast<void*>(std::to_address(end))));
	std::size_t const cped(istrvw.str().copy(pb,pe-pb));
	istrvw.str().remove_prefix(cped);
	return begin+cped*sizeof(*begin)/sizeof(typename T::value_type);
}

template<typename T>
inline constexpr typename T::value_type get(basic_istring_view<T>& istrvw)
{
#ifdef __EXCEPTIONS
	if(istrvw.empty())
		throw eof();
#endif
	auto ch(istrvw.str().front());
	istrvw.str().remove_prefix(1);
	return ch;
}
template<typename T>
inline constexpr std::pair<typename T::value_type,bool> try_get(basic_istring_view<T>& istrvw)
{
	if(istrvw.empty())
		return {0,true};
	auto ch(istrvw.str().front());
	istrvw.str().remove_prefix(1);
	return {ch,false};
}

using istring_view = basic_istring_view<std::string_view>;

}