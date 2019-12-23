#pragma once

namespace fast_io::details
{
inline constexpr std::pair<std::size_t,std::size_t> cal_base_pw_size(std::size_t u)
{
	std::size_t retch(0);
	std::size_t retpw(0);

	std::size_t chars(0);
	std::size_t pw(1);
	for(;pw*chars<512;++chars)
	{
		retch=chars;
		retpw=pw;
		pw*=u;
	}
	if(retch<2)
		return {2,u*u};
	else
		return {retch,retpw};
}

template<std::size_t base,bool upper>
inline constexpr auto cal_content()
{
	constexpr auto val(cal_base_pw_size(base));
	constexpr std::size_t chars(val.first);
	static_assert(1<chars,"table width must be larger than 1");
	constexpr std::size_t pw(val.second);
	std::array<std::array<char,chars>,pw> vals{};
	for(std::size_t i(1);i<pw;++i)
	{
		auto& val(vals[i]);
		val=vals[i-1];
		std::size_t j(chars);
		for(;j--;)
		{
			if(val[j]==base-1)
				val[j]=0;
			else
				break;
		}
		++val[j];
	}
	for(auto &e : vals)
		for(auto &e1 : e)
			if constexpr(10<base)
			{
				if(e1<10)
					e1+=0x30;
				else
				{
					if constexpr(upper)
						e1+=0x41-10;
					else
						e1+=0x61-10;
				}
			}
			else
				e1+=0x30;
	return vals;
}

template<std::size_t base,bool upper>
struct shared_static_base_table
{
	inline static constexpr auto table=cal_content<base,upper>();
};
}
