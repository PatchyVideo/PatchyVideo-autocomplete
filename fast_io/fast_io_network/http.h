#pragma once
#include<unordered_map>

namespace fast_io
{

// i think this should use coroutine. Let's wait until C++20

template<character_input_stream input>
inline void get_http_header_split(input& in,std::basic_string<typename input::char_type> &str)
{
	str.clear();
	for(decltype(try_get(in)) ch;!(ch=try_get(in)).second&&!details::isspace(ch.first)&&ch.first!='\r'&&ch.first!=':';str.push_back(ch.first));
}

template<fast_io::character_input_stream input>
inline constexpr auto scan_http_header(input& in)
{
	std::unordered_map<std::string,std::string> results;
	for(std::string str,a,b;;)
	{
		getline(in,str);
		if(str.size()<2)
			return results;
		fast_io::istring_view isv(str);
		get_http_header_split(isv,a);
		if(a.back()==':')
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