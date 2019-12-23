#pragma once


namespace fast_io
{

template<typename T>
class basic_ospan
{
	T s;
public:
	typename T::pointer internal_pointer;
	using value_type = T;
	using char_type = typename T::value_type;
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	constexpr basic_ospan(Args&& ...args):s(std::forward<Args>(args)...),internal_pointer(s.data()){}
	constexpr auto& span()
	{
		return s;
	}
	constexpr void clear(){internal_pointer=s.data();}
};

template<typename T>
[[nodiscard]] inline constexpr std::size_t osize(basic_ospan<T>& ob)
{
	return ob.internal_pointer-ob.span().data();
}

template<typename T>
[[nodiscard]] inline constexpr auto oreserve(basic_ospan<T>& ob,std::size_t size)
{
	return ob.internal_pointer+=size;
}

template<typename T>
inline constexpr void orelease(basic_ospan<T>& ob,std::size_t size)
{
	ob.internal_pointer-=size;
}

template<typename T,std::contiguous_iterator Iter>
inline constexpr void send(basic_ospan<T>& ob,Iter cbegin,Iter cend)
{
	using char_type = typename T::value_type;
	ob.internal_pointer=std::copy(static_cast<char_type const*>(static_cast<void const*>(std::to_address(cbegin))),
		static_cast<char_type const*>(static_cast<void const*>(std::to_address(cend))),ob.internal_pointer);
}
template<typename T>
inline constexpr void put(basic_ospan<T>& ob,typename T::value_type ch)
{
	*ob.internal_pointer=ch;
	++ob.internal_pointer;
}

template<typename T>
inline constexpr void flush(basic_ospan<T>&){}

template<typename T>
inline constexpr void fill_nc(basic_ospan<T>& os,std::size_t count,typename T::value_type const& ch)
{
	os.internal_pointer=std::fill_n(os.internal_pointer,count,ch);
}

template<output_stream output,typename T>
inline constexpr void print_define(output& out,basic_ospan<T> s)
{
	send(out,s.span().data(),s.internal_pointer);
}

template<typename T>
[[nodiscard]] inline constexpr basic_istring_view<std::basic_string_view<typename T::value_type>> to_istring_view(basic_ospan<T> s)
{
	return {s.span().data(),static_cast<std::size_t>(s.internal_pointer-s.span().data())};
}

template<typename T>
[[nodiscard]] inline constexpr std::basic_string_view<typename T::value_type> to_string_view(basic_ospan<T> s)
{
	return {s.span().data(),static_cast<std::size_t>(s.internal_pointer-s.span().data())};
}

}

#ifdef __cpp_lib_span

#include<span>

namespace fast_io
{
	using ospan = basic_ospan<std::span<char>>;
}

#endif