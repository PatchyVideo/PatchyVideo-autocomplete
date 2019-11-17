#pragma once
#include<chrono>

namespace fast_io
{

//We use seconds since seconds is the standard unit of SI
template<output_stream output,typename Rep,typename Period>
void print_define(output& out, std::chrono::duration<Rep,Period> const& duration)
{
	print(out,std::chrono::duration_cast<std::chrono::duration<double>>(duration).count());	//should use shortest however ryu does not support that
}

template<output_stream output,typename Clock,typename Duration>
void print_define(output& out, std::chrono::time_point<Clock,Duration> const& tmp)
{
	print(out,tmp.time_since_epoch());
}


}