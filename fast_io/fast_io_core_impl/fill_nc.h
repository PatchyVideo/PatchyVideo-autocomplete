#pragma once

namespace fast_io
{

template<character_output_stream output>
inline constexpr void fill_nc(output& out,std::size_t count,typename output::char_type const& ch)
{
	for(std::size_t i(0);i!=count;++i)
		put(out,ch);
}

}