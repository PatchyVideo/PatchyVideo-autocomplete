#pragma once

namespace fast_io
{
//potential constexpr in the future if std::u8string can be constexpr

template<typename T=std::u8string,typename... Args>
inline constexpr T concat(Args&& ...args)
{
	basic_ostring<T> t;
	print(t,std::forward<Args>(args)...);
	return std::move(t.str());
}

template<typename T=std::u8string,typename... Args>
inline constexpr T concatln(Args&& ...args)
{
	basic_ostring<T> t;
	println(t,std::forward<Args>(args)...);
	return std::move(t.str());
}

template<typename T=std::u8string,typename... Args>
inline constexpr T format(std::u8string_view format,Args&& ...args)
{
	basic_ostring<T> t;
	fprint(t,format,std::forward<Args>(args)...);
	return std::move(t.str());
}

template<typename T,typename... Args>
inline constexpr void in_place_to(T& t,Args&& ...args)
{
	basic_ostring<std::u8string> os;
	print(os,std::forward<Args>(args)...);
	basic_istring_view<std::u8string_view> is(os.str());
	scan(is,t);
}

template<typename... Args>
inline constexpr void in_place_to(std::u8string& t,Args&& ...args)
{
	basic_ostring<std::u8string> os(std::move(t));
	os.clear();
	print(os,std::forward<Args>(args)...);
	t=std::move(os.str());
}

template<typename T,typename... Args>
inline constexpr auto to(Args&& ...args)
{
	T t;
	in_place_to(t,std::forward<Args>(args)...);
	return t;
}
}