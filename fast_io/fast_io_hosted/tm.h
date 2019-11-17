#pragma once
#include<chrono>

namespace fast_io
{
template<character_output_stream output,typename T>
requires std::same_as<std::tm,std::remove_cvref_t<T>>
inline constexpr void print_define(output& out,T const& a)
{
	fprint(out,"%/%/% [yday:%] (wday:%) %:%:%",a.tm_year+1900,a.tm_mon+1,a.tm_mday,a.tm_yday,a.tm_wday,a.tm_hour,a.tm_min,a.tm_sec);
	if(0<a.tm_isdst)
		print(out," DST");
	else if(a.tm_isdst<0)
		print(out," ???");
}
}