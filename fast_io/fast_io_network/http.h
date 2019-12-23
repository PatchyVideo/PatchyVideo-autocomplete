#pragma once
#include<unordered_map>

namespace fast_io
{

// i think this should use coroutine. Let's wait until C++20

template<character_input_stream input>
inline void get_http_header_split(input& in,std::basic_string<typename input::char_type> &str)
{
	str.clear();
	for(decltype(get<true>(in)) ch;!(ch=get<true>(in)).second&&!details::isspace(ch.first)&ch.first!=0xD&ch.first!=0x3a;str.push_back(ch.first));
}

template<fast_io::character_input_stream input>
inline constexpr auto scan_http_header(input& in)
{
	using string_type = std::basic_string<typename input::char_type>;
	std::unordered_map<string_type,string_type> results;
	for(string_type str,a,b;;)
	{
		getline(in,str);
		if(str.size()<2)
			return results;
		fast_io::basic_istring_view<std::basic_string_view<typename input::char_type>> isv(str);
		get_http_header_split(isv,a);
		if(a.back()==0x3a)
			a.pop_back();
		getcarriage(isv,b);
		results.emplace(std::move(a),std::move(b));
	}
}
template<fast_io::character_input_stream input>
inline constexpr void skip_http_header(input& in)
{
	while(1<skip_line(in));
}


}