#pragma once

namespace fast_io
{

template<character_input_stream input>
inline void scan_define(input& in,std::basic_string<typename input::char_type> &str)
{
	str.clear();
	str.push_back(eat_space_get(in));
	for(decltype(try_get(in)) ch;!(ch=try_get(in)).second&&!details::isspace(ch.first);str.push_back(ch.first));
}

template<character_input_stream input>
inline void getline(input& in,std::basic_string<typename input::char_type> &str)
{
	str.clear();
	for(decltype(try_get(in)) ch;!(ch=try_get(in)).second&&ch.first!='\n';str.push_back(ch.first));
}

template<character_input_stream input>
inline constexpr std::size_t skip_line(input& in)
{
	std::size_t skipped(0);
	for(decltype(try_get(in)) ch;!(ch=try_get(in)).second&&ch.first!='\n';++skipped);
	return skipped;
}


template<character_input_stream input>
inline void getcarriage(input& in,std::basic_string<typename input::char_type> &str)
{
	str.clear();
	for(decltype(try_get(in)) ch;!(ch=try_get(in)).second&&ch.first!='\r';str.push_back(ch.first));
}

template<character_input_stream input>
inline void getwhole(input& in,std::basic_string<typename input::char_type> &str)
{
	str.clear();
	for(decltype(try_get(in)) ch;!(ch=try_get(in)).second;str.push_back(ch.first));
}

template<character_output_stream output,std::size_t indent_width,bool left,char ch,typename T>
inline constexpr void print_define(output& out,manip::width<indent_width,left,ch,T const> a)
{
	basic_ostring<std::basic_string<typename output::char_type>> bas;
	print(bas,a.reference);
	std::size_t const size(bas.str().size());
	if(size<indent_width)
	{
		if constexpr(left)
		{
			print(out,bas.str());
			fill_nc(out,indent_width-size,ch);
		}	
		else
		{
			fill_nc(out,indent_width-size,ch);
			print(out,bas.str());
		}
	}
	else
		print(out,bas.str());
}

}