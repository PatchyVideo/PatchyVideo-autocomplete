#pragma once
#include"ryu/ryu.h"

namespace fast_io
{
/*
namespace details
{

template<character_output_stream output,std::floating_point T>
inline bool constexpr output_inf(output& out,T e)
{
	if(e==std::numeric_limits<T>::signaling_NaN()||e==std::numeric_limits<T>::quiet_NaN())
	{
		print(out,"nan");
		return true;		
	}
	else if(e==std::numeric_limits<T>::infinity())
	{
		print(out,"inf");
		return true;
	}
	return false;
}

template<std::floating_point F,std::unsigned_integral U>
inline constexpr F mpow(F const &f,U const& u)
{
	if(u==0)
		return 1;
	else if(u==1)
		return f;
	else
	{
		F d(mpow(f,u>>1));
		d*=d;
		if(u&1)
			return d*f;
		else
			return d;
	}
}


template<character_output_stream output,std::floating_point T>
inline void output_fixed_floats(output& out,T e,std::size_t precision)
{
	auto u(std::floor(e));
	e-=u;
	auto gggg(details::mpow(static_cast<T>(10),precision));
	auto temp(e*gggg);
	auto p(temp*10);
	auto ptg(std::floor(temp));
	auto const mdg(p-ptg*10);
	if(mdg<5||(mdg==5&&((ptg-std::floor(ptg/2)*2)==0||std::floor(p)!=p)))
	{
		std::basic_string<typename output::char_type> bas;
		do
		{
			temp = std::floor(u/10);
			std::make_unsigned_t<typename output::char_type> v(u-temp*10);
			if(v<10)
				bas.push_back(v+48);
			else
				bas.push_back(48);
		}
		while((u=temp));
		for(std::size_t i(bas.size());i--;put(out,bas[i]));
		if(!precision)
			return;
		put(out,0x2E);
		bas.clear();
		do
		{
			temp = std::floor(ptg/10);
			std::make_unsigned_t<typename output::char_type> v(ptg-temp*10);
			if(v<10)
				bas.push_back(v+48);
			else
				bas.push_back(48);
		}
		while((ptg=temp)&&bas.size()<=precision);
		for(std::size_t i(bas.size());i<precision;++i)
			put(out,48);
		for(std::size_t i(bas.size());i--;put(out,bas[i]));
	}
	else
	{
		++ptg;
		std::basic_string<typename output::char_type> bas;
		bool hasone(false);
		do
		{
			if(precision<=bas.size())
				break;
			temp = std::floor(ptg/10);
			std::make_unsigned_t<typename output::char_type> v(ptg-temp*10);
			if(v<10)
				bas.push_back(v+48);
			else
				bas.push_back(48);
			if(v)
				hasone=true;
		}
		while((ptg=temp));
		std::basic_string<typename output::char_type> bas2;
		if(!hasone)
		{
			++u;
			bas.clear();
			bas2=std::move(bas);
		}
		do
		{
			temp = std::floor(u/10);
			std::make_unsigned_t<typename output::char_type> v(u-temp*10);
			if(v<10)
				bas2.push_back(v+48);
			else
				bas2.push_back(48);
		}
		while((u=temp));
		if(bas2.empty())
			put(out,0x30);
		else
			for(std::size_t i(bas2.size());i--;put(out,bas2[i]));
		if(!precision)
			return;
		put(out,0x2E);
		if(!hasone)
			bas.clear();
		for(std::size_t i(bas.size());i<precision;++i)
			put(out,0x30);
		for(std::size_t i(bas.size());i--;put(out,bas[i]));
	}
}

}

template<character_output_stream output,std::size_t precision,std::floating_point T>
inline void print_define(output& out,manip::scientific<precision,T const> a)
{
	auto e(a.reference);
	if(e==0)	//if e==0 then log10 is UNDEFINED
	{
		put(out,0x30);
		if(a.precision)
		{
			put(out,0x2E);
			for(std::size_t i(0);i!=precision;++i)
				put(out,0x30);
		}
		print(out,"e0");
		return;
	}
	if(e<0)
	{
		e=-e;
		put(out,0x2d);
	}
	if(details::output_inf(out,e))
		return;
	auto x(std::floor(std::log10(e)));
	details::output_fixed_floats(out,e*std::pow(10,-x),precision);
	put(out,0x65);
	if(x<0)
	{
		put(out,0x2d);
		x=-x;
	}
	print(out,static_cast<std::uint64_t>(x));
}

template<character_output_stream output,std::size_t precision,std::floating_point T>
inline void print_define(output& out,manip::shortest<std::size_t,T const> a)
{
	auto e(a.reference);
	if(e==0)	//if e==0 then log10 is UNDEFINED
	{
		put(out,0x30);
		return;
	}
	if(e<0)
	{
		e=-e;
		put(out,0x2d);
	}
	if(details::output_inf(out,e))
		return;
	auto x(std::floor(std::log10(e)));
	{
	auto fix(std::fabs(x)<=precision);
	basic_ostring<std::basic_string<typename output::char_type>> bas;
	if(fix)
		details::output_fixed_floats(bas,e,precision);
	else
		details::output_fixed_floats(bas,e*std::pow(10,-x),precision);
	auto& str(bas.str());
	if(str.find(0x2E)!=std::string::npos)
	{
		for(;!str.empty()&&str.back()==0x30;str.pop_back());
		if(!str.empty()&&str.back()==0x2E)
			str.pop_back();
		print(out,str);
	}		
	if(fix)
		return;
	}
	if(x==0)
		return;
	put(out,0x65);
	if(x<0)
	{
		put(out,0x2d);
		x=-x;
	}
	print(out,static_cast<std::uint64_t>(x));
}

template<character_output_stream soutp,std::floating_point T>
inline void print_define(soutp &output,T const& p)
{
	print(output,shortest<6>(p));
}
*/

template<output_stream output,std::size_t precision,std::floating_point T>
inline void print_define(output& out,manip::fixed<precision,T const> a)
{

	std::size_t constexpr reserved_size(precision+325);
	if constexpr(buffer_output_stream<output>)
	{

		auto reserved(oreserve(out,reserved_size));
		if constexpr(std::is_pointer_v<decltype(reserved)>)
		{
			if(reserved)
			{
				auto start(reserved-reserved_size);
				orelease(out,reserved-details::ryu::output_fixed<precision>(start,static_cast<double>(a.reference)));
				return;
			}
		}
		else
		{
			auto start(reserved-reserved_size);
			orelease(out,reserved-details::ryu::output_fixed<precision>(start,static_cast<double>(a.reference)));
			return;
		}
	}
	if constexpr (precision<325)
	{
		std::array<typename output::char_type,reserved_size> array;
		send(out,array.data(),details::ryu::output_fixed<precision>(array.data(),static_cast<double>(a.reference)));
	}
	else
	{
		std::basic_string<typename output::char_type> str(reserved_size);
		send(out,str.data(),details::ryu::output_fixed<precision>(str.data(),static_cast<double>(a.reference)));
	}
}

template<output_stream output,std::size_t precision,bool uppercase_e,std::floating_point T>
inline void print_define(output& out,manip::scientific<precision,uppercase_e,T const> a)
{

	std::size_t constexpr reserved_size(precision+10);
	if constexpr(buffer_output_stream<output>)
	{

		auto reserved(oreserve(out,reserved_size));
		if constexpr(std::is_pointer_v<decltype(reserved)>)
		{
			if(reserved)
			{
				auto start(reserved-reserved_size);
				orelease(out,reserved-details::ryu::output_fixed<precision,true,uppercase_e>(start,static_cast<double>(a.reference)));
				return;
			}
		}
		else
		{
			auto start(reserved-reserved_size);
			orelease(out,reserved-details::ryu::output_fixed<precision,true,uppercase_e>(static_cast<double>(a.reference)));
			return;
		}
	}
	if constexpr (precision<325)
	{
		std::array<typename output::char_type,reserved_size> array;
		send(out,array.data(),details::ryu::output_fixed<precision,true,uppercase_e>(array.data(),static_cast<double>(a.reference)));
	}
	else
	{
		std::basic_string<typename output::char_type> str(reserved_size);
		send(out,str.data(),details::ryu::output_fixed<precision,true,uppercase_e>(str.data(),static_cast<double>(a.reference)));
	}
}

template<output_stream output,std::floating_point T>
inline void print_define(output& out,manip::fixed_shortest<T const> a)
{

	std::size_t constexpr reserved_size(325);
	if constexpr(buffer_output_stream<output>)
	{

		auto reserved(oreserve(out,reserved_size));
		if constexpr(std::is_pointer_v<decltype(reserved)>)
		{
			if(reserved)
			{
				auto start(reserved-reserved_size);
				orelease(out,reserved-details::ryu::output_shortest<false,1>(start,static_cast<double>(a.reference)));
				return;
			}
		}
		else
		{
			auto start(reserved-reserved_size);
			orelease(out,reserved-details::ryu::output_shortest<false,1>(start,static_cast<double>(a.reference)));
			return;
		}
	}
	std::array<typename output::char_type,reserved_size> array;
	send(out,array.data(),details::ryu::output_shortest<false,1>(array.data(),static_cast<double>(a.reference)));
}

template<output_stream output,bool uppercase_e,std::floating_point T>
inline void print_define(output& out,manip::scientific_shortest<uppercase_e,T const> a)
{

	std::size_t constexpr reserved_size(30);
	if constexpr(buffer_output_stream<output>)
	{

		auto reserved(oreserve(out,reserved_size));
		if constexpr(std::is_pointer_v<decltype(reserved)>)
		{
			if(reserved)
			{
				auto start(reserved-reserved_size);
				orelease(out,reserved-details::ryu::output_shortest<uppercase_e,2>(start,static_cast<double>(a.reference)));
				return;
			}
		}
		else
		{
			auto start(reserved-reserved_size);
			orelease(out,reserved-details::ryu::output_shortest<uppercase_e,2>(start,static_cast<double>(a.reference)));
			return;
		}
	}
	std::array<typename output::char_type,reserved_size> array;
	send(out,array.data(),details::ryu::output_shortest<uppercase_e,2>(array.data(),static_cast<double>(a.reference)));
}

template<output_stream output,bool uppercase_e,std::floating_point T>
inline void print_define(output& out,manip::shortest_shortest<uppercase_e,T const> a)
{

	std::size_t constexpr reserved_size(30);
	if constexpr(buffer_output_stream<output>)
	{

		auto reserved(oreserve(out,reserved_size));
		if constexpr(std::is_pointer_v<decltype(reserved)>)
		{
			if(reserved)
			{
				auto start(reserved-reserved_size);
				orelease(out,reserved-details::ryu::output_shortest<uppercase_e>(start,static_cast<double>(a.reference)));
				return;
			}
		}
		else
		{
			auto start(reserved-reserved_size);
			orelease(out,reserved-details::ryu::output_shortest<uppercase_e>(start,static_cast<double>(a.reference)));
			return;
		}
	}
	std::array<typename output::char_type,reserved_size> array;
	send(out,array.data(),details::ryu::output_shortest<uppercase_e>(array.data(),static_cast<double>(a.reference)));
}

template<output_stream output,std::floating_point T>
inline void print_define(output& out,T a)
{
	std::size_t constexpr reserved_size(30);
	if constexpr(buffer_output_stream<output>)
	{

		auto reserved(oreserve(out,reserved_size));
		if constexpr(std::is_pointer_v<decltype(reserved)>)
		{
			if(reserved)
			{
				auto start(reserved-reserved_size);
				orelease(out,reserved-details::ryu::output_shortest<false>(start,static_cast<double>(a)));
				return;
			}
		}
		else
		{
			auto start(reserved-reserved_size);
			orelease(out,reserved-details::ryu::output_shortest<false>(start,static_cast<double>(a)));
			return;
		}
	}
	std::array<typename output::char_type,reserved_size> array;
	send(out,array.data(),details::ryu::output_shortest<false>(array.data(),static_cast<double>(a)));
}

template<character_input_stream input,std::floating_point T>
inline constexpr void scan_define(input& in,T &t)
{
	t=static_cast<std::remove_cvref_t<T>>(details::ryu::input_floating<double>(in));
}

}
