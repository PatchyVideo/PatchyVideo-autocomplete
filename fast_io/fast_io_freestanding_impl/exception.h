#pragma once

namespace fast_io
{

template<character_output_stream output>
inline void print_define(output& out,std::exception const &e)
{
	print(out,e.what());
}

template<character_output_stream output>
inline void print_define(output& out,std::system_error const &e)
{
	auto const& code(e.code());
	print(out,"std::system_error, value:",code.value(),"\tmessage:",code.message());
}

}