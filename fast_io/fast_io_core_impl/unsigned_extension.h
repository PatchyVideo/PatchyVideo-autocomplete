#pragma once

#if defined(_MSC_VER) && defined(_M_X64)
#include<intrin.h>
#endif

namespace fast_io
{
template<typename T>
struct basic_unsigned_extension
{
	T low={},high={};
	using value_type = T;
	constexpr basic_unsigned_extension()=default;
	template<typename U>
	requires std::constructible_from<T,U>&&(!std::same_as<U,basic_unsigned_extension<T>>)
	constexpr explicit basic_unsigned_extension(U && a):low(std::forward<U>(a)){}
	template<typename U,typename U1>
	requires std::constructible_from<T,U>&&std::constructible_from<T,U1>
	constexpr basic_unsigned_extension(U && a,U1 && b):low(std::forward<U>(a)),high(std::forward<U1>(b)){}
	inline explicit constexpr operator bool()
	{
		return low||high;
	}

	inline constexpr bool operator[](std::size_t n) const
	{
		return (static_cast<std::uint8_t const*>(static_cast<void const*>(this))[n / 8] >> (n % 8)) & 1;
	}
	inline constexpr bool front() const
	{
		return (*static_cast<std::uint8_t const*>(static_cast<void const*>(this)))&1;
	}
	inline explicit constexpr operator T() const
	{
		return low;
	}
	inline explicit constexpr operator std::uint64_t() const requires(sizeof(T)!=8)
	{
		return static_cast<std::uint64_t>(low);
	}
	inline explicit constexpr operator std::uint32_t() const
	{
		return static_cast<std::uint32_t>(low);
	}
};


template<typename T>
inline constexpr T const& low(basic_unsigned_extension<T> const&a)
{
	return a.low;
}

template<typename T>
inline constexpr T const& high(basic_unsigned_extension<T> const&a)
{
	return a.high;
}

inline constexpr std::uint32_t low(std::uint64_t a)
{
	return a&UINT32_MAX;
}

inline constexpr std::uint32_t high(std::uint64_t a)
{
	return a>>32;
}

template<typename T>
inline constexpr basic_unsigned_extension<basic_unsigned_extension<T>> construct(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return {a,b};
}

#ifdef __SIZEOF_INT128__
inline constexpr std::uint64_t low(__uint128_t a)
{
	return a&UINT64_MAX;
}

inline constexpr std::uint64_t high(__uint128_t a)
{
	return a>>64;
}

inline constexpr __uint128_t construct_unsigned_extension(std::uint64_t a,std::uint64_t b)
{
	return (static_cast<__uint128_t>(b)<<64)|a;
}

inline constexpr basic_unsigned_extension<__uint128_t> construct_unsigned_extension(__uint128_t a,__uint128_t b)
{
	return {a,b};
}

template<typename T>
requires (!std::unsigned_integral<T>&&std::same_as<T,__uint128_t>)
inline constexpr T reset_high(T& a)
{
	T temp(a>>64);
	a&=UINT64_MAX;
	return temp;
}
inline constexpr __uint128_t merge(__uint128_t a,__uint128_t b)
{
	return a|(b<<64);
}
#else
inline constexpr basic_unsigned_extension<std::uint64_t> construct_unsigned_extension(std::uint64_t a,std::uint64_t b)
{
	return {a,b};
}
#endif

inline constexpr std::uint64_t merge(std::uint64_t a,std::uint64_t b)
{
	return a|(b<<32);
}

template<typename T>
inline constexpr basic_unsigned_extension<T> merge(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return {a.low,b.low};
}

template<typename T>
inline constexpr T reset_high(basic_unsigned_extension<T>& a)
{
	T temp(a.high);
	a.high={};
	return temp;
}

template<typename T>
requires std::unsigned_integral<T>
inline constexpr auto reset_high(T& a)
{
	constexpr T bytes(sizeof(T)*4),bytes_m1((static_cast<T>(1)<<bytes)-1);
	auto temp(a>>bytes);
	a&=bytes_m1;
	return temp;
}


template<typename T>
inline constexpr bool operator==(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return a.low==b.low&&a.high==b.high;
}

template<typename T>
inline constexpr bool operator!=(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return a.low!=b.low||a.high!=b.high;
}

template<typename T>
inline constexpr bool operator<(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return a.high==b.high?a.low<b.low:a.high<b.high;
}

template<typename T>
inline constexpr bool operator>(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return b<a;
}

template<typename T>
inline constexpr bool operator<=(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return !(b<a);
}

template<typename T>
inline constexpr bool operator>=(basic_unsigned_extension<T> const& a,basic_unsigned_extension<T> const& b)
{
	return !(a<b);
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator++(basic_unsigned_extension<T>& a)
{
	if(!++a.low)
		++a.high;
	return a;
}
template<typename T>
inline constexpr basic_unsigned_extension<T>& operator--(basic_unsigned_extension<T>& a)
{
	if(!a.low)
		--a.high;
	--a.low;
	return a;
}

template<typename T,typename P>
requires
#ifdef __SIZEOF_INT128__
((std::unsigned_integral<T>||std::same_as<T,__uint128_t>)&&(std::unsigned_integral<P>||std::same_as<P,__uint128_t>))
#else
(std::unsigned_integral<T>&&std::unsigned_integral<P>)
#endif
inline constexpr bool add_carry_assignment(bool carry_flag,T& a,P b)
{
	auto const temp(carry_flag+a+b);
	bool const carry(temp<a);
	a=temp;
	return carry;
}

template<typename T>
inline constexpr bool add_carry_assignment(bool carry_flag,basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	return add_carry_assignment(add_carry_assignment(carry_flag,a.low,b.low),a.high,b.high);
}

template<typename T,typename P>
requires
#ifdef __SIZEOF_INT128__
(std::unsigned_integral<P>||std::same_as<P,__uint128_t>)
#else
(std::unsigned_integral<P>)
#endif
inline constexpr bool add_carry_assignment(bool carry_flag,basic_unsigned_extension<T>& a,P value)
{
	return add_carry_assignment(add_carry_assignment(carry_flag,a.low,value),a.high,0u);
}
/*
template<typename T>
inline constexpr bool add_carry_assignment_single(bool carry_flag,basic_unsigned_extension<T>& a,T value)
{
	return false;
//	return add_carry_assignment(add_carry_assignment(carry_flag,a.low,value),a.high,0u);
}
*/

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator+=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	add_carry_assignment(false,a,b);
	return a;
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator+=(basic_unsigned_extension<T>& a,T const& b)
{
	add_carry_assignment(add_carry_assignment(false,a.low,b),a.high,0u);
	return a;
}

template<typename T,typename P>
inline constexpr basic_unsigned_extension<T> operator+(basic_unsigned_extension<T> a,P&& b)
{
	return a+=std::forward<P>(b);
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator-=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	auto const added_low(a.low-b.low);
	a.high-=b.high;
	if(a.low<added_low)
		--a.high;
	a.low=added_low;
	return a;
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T>& operator-=(basic_unsigned_extension<T>& a,P const& b)
{
	auto const added_low(a.low-b);
	if(a.low<added_low)
		--a.high;
	a.low = added_low;
	return a;
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T> operator-(basic_unsigned_extension<T> a,P&& b)
{
	return a-=std::forward<P>(b);
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator|=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	a.low|=b.low;
	a.high|=b.high;
	return a;
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T>& operator|=(basic_unsigned_extension<T>& a,P const& b)
{
	a.low|=b;
	return a;
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator^=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	a.low^=b.low;
	a.high^=b.high;
	return a;
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T>& operator^=(basic_unsigned_extension<T>& a,P const& b)
{
	a.low^=b;
	return a;
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator&=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	a.low&=b.low;
	a.high&=b.high;
	return a;
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T>& operator&=(basic_unsigned_extension<T>& a,P const& b)
{
	a.low&=b;
	return a;
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T> operator|(basic_unsigned_extension<T> a,P&& b)
{
	return a|=std::forward<P>(b);
}
template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T> operator^(basic_unsigned_extension<T> a,P&& b)
{
	return a^=std::forward<P>(b);
}
template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T> operator&(basic_unsigned_extension<T> a,P&& b)
{
	return a&=std::forward<P>(b);
}

#if defined(_MSC_VER) && defined(_M_X64)

inline constexpr basic_unsigned_extension<std::uint64_t>& operator<<=(basic_unsigned_extension<std::uint64_t>& a,std::size_t shift)
{
	if(shift<128) [[likely]]
	{
		a.high=__shiftleft128(a.low,a.high,static_cast<unsigned char>(shift));
		a.low<<=shift;
	}
	else
		a.high=a.low=0;
	return a;
}
inline constexpr basic_unsigned_extension<std::uint64_t>& operator>>=(basic_unsigned_extension<std::uint64_t>& a,std::size_t shift)
{
	if(shift<128) [[likely]]
	{
		a.low=__shiftright128(a.low,a.high,static_cast<unsigned char>(shift));
		a.high>>=shift;
	}
	else
		a.high=a.low=0;
	return a;
}
#endif

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator<<=(basic_unsigned_extension<T>& a,std::size_t shift)
{
	constexpr std::size_t total_bytes(sizeof(T)*8);
	if(!shift)
		return a;
	if(shift<total_bytes)
	{
		a.high<<=shift;
		a.high|=a.low>>(total_bytes-shift);
		a.low<<=shift;
	}
	else if(shift<(total_bytes<<1))
	{
		a.high=a.low<<(shift-total_bytes);
		a.low=T();
	}
	else
		a=basic_unsigned_extension<T>();
	return a;
}

template<typename T>
inline constexpr basic_unsigned_extension<T> operator<<(basic_unsigned_extension<T> a,std::size_t shift)
{
	return a<<=shift;
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator>>=(basic_unsigned_extension<T>& a,std::size_t shift)
{
	constexpr std::size_t total_bytes(sizeof(T)*8);
	if(!shift)
		return a;
	if(shift<total_bytes)
	{
		a.low>>=shift;
		a.low|=a.high<<(total_bytes-shift);
		a.high>>=shift;
	}
	else if(shift<(total_bytes<<1))
	{
		a.low=a.high>>(shift-total_bytes);
		a.high=T();
	}
	else
		a=basic_unsigned_extension<T>();
	return a;
}

template<typename T>
inline constexpr basic_unsigned_extension<T> operator>>(basic_unsigned_extension<T> a,std::size_t shift)
{
	return a>>=shift;
}


template<typename T>
requires std::same_as<std::uint32_t,T>
inline constexpr std::uint64_t mul_extend(T const& a,T const& b)
{
	return static_cast<std::uint64_t>(a)*b;
}

#ifdef __SIZEOF_INT128__
template<typename T>
requires std::same_as<std::uint64_t,T>
inline constexpr __uint128_t mul_extend(T const& a,T const& b)
{
	return static_cast<__uint128_t>(a)*b;
}
#elif defined(_MSC_VER) && defined(_M_X64)
template<typename T>
requires std::same_as<std::uint64_t,T>
inline constexpr basic_unsigned_extension<std::uint64_t> mul_extend(T const& a,T const& b)
{
	std::uint64_t high;
	std::uint64_t low(_umul128(a,b,std::addressof(high)));
	return {low,high};
}
#endif


template<typename T>
inline constexpr basic_unsigned_extension<T> mul_extend(T const& a,T const& b)
{
	decltype(auto) a0(low(a));
	decltype(auto) a1(high(a));
	decltype(auto) b0(low(b));
	decltype(auto) b1(high(b));
	decltype(auto) c0(mul_extend(a0,b0));
	decltype(auto) c1(mul_extend(a1,b0)+mul_extend(a0,b1));
	c1+=reset_high(c0);
	T c2(mul_extend(a1,b1));
	c2+=reset_high(c1);
	return {merge(c0,c1),c2};	//RVO
}
template<typename T>
inline constexpr T mul_high(T const& a,T const& b)
{
	decltype(auto) a0(low(a));
	decltype(auto) a1(high(a));
	decltype(auto) b0(low(b));
	decltype(auto) b1(high(b));
	T c1(mul_extend(a0,b1));
	c1+=mul_extend(a1,b0);
	c1+=high(mul_extend(a0,b0));
	return mul_extend(a1,b1)+reset_high(c1);	//RVO
}

template<typename T,typename U>
requires std::unsigned_integral<U>&&std::same_as<std::common_type_t<U,std::uint32_t>,std::uint32_t>
inline constexpr basic_unsigned_extension<T>& operator*=(basic_unsigned_extension<T>& a,U b)
{
	T c0(low(a.low));
	c0*=b;
	T c1(high(a.low));
	c1*=b;
	c1+=reset_high(c0);
	T c2(low(a.high));
	c2*=b;
	c2+=reset_high(c1);
	T c3(high(a.high));
	c3*=b;
	c3+=reset_high(c2);
	return a=basic_unsigned_extension<T>(merge(c0,c1),merge(c2,c3));
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator*=(basic_unsigned_extension<T>& a,T const& b)
{
	decltype(auto) a0(low(a.low));
	decltype(auto) a1(high(a.low));
	decltype(auto) a2(low(a.high));
	decltype(auto) a3(high(a.high));
	decltype(auto) b0(low(b));
	decltype(auto) b1(high(b));
	T c0(mul_extend(a0,b0));
	T c1(mul_extend(a0,b1)+mul_extend(a1,b0)+reset_high(c0));
	T c2(mul_extend(a1,b1)+mul_extend(a2,b0)+reset_high(c1));
	T c3(mul_extend(a2,b1)+mul_extend(a3,b0)+reset_high(c2));
	return a={merge(c0,c1),merge(c2,c3)};
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator*=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	decltype(auto) a0(low(a.low));
	decltype(auto) a1(high(a.low));
	decltype(auto) a2(low(a.high));
	decltype(auto) a3(high(a.high));
	decltype(auto) b0(low(b.low));
	decltype(auto) b1(high(b.low));
	decltype(auto) b2(low(b.high));
	decltype(auto) b3(high(b.high));
	T c0(mul_extend(a0,b0));
	T c1(mul_extend(a0,b1)+mul_extend(a1,b0)+reset_high(c0));
	T c2(mul_extend(a0,b2)+mul_extend(a1,b1)+mul_extend(a2,b0)+reset_high(c1));
	T c3(mul_extend(a0,b3)+mul_extend(a1,b2)+mul_extend(a2,b1)+mul_extend(a3,b0)+reset_high(c2));
	return a={merge(c0,c1),merge(c2,c3)};
}

template<typename T,typename P>
requires std::constructible_from<basic_unsigned_extension<T>,P>
inline constexpr basic_unsigned_extension<T> operator*(basic_unsigned_extension<T> a,P&& b)
{
	return a*=std::forward<P>(b);
}

template<typename T>
inline constexpr auto div_mod(basic_unsigned_extension<T> const& lhs, basic_unsigned_extension<T> const& rhs)	//C++17 copy elision
{
	std::pair<basic_unsigned_extension<T>, basic_unsigned_extension<T>> value;
	auto& quotient(value.first);
	auto& remainder(value.second);
	for (std::size_t i(sizeof(T)*16); i--;)
	{
		quotient <<= 1;
		remainder <<= 1;
		if (lhs[i])
			++remainder;
		if (remainder >= rhs) {
			remainder -= rhs;
			++quotient;
		}
	}
	return value;
}

template<typename T>
inline uint32_t in_place_div_mod(basic_unsigned_extension<T>& a,std::uint32_t value)
{
	std::array<std::uint32_t,sizeof(T)/2> v;
	memcpy(v.data(),std::addressof(a),sizeof(v));		//should use std::bit_cast instead. However, current compilers haven't implemented std::bit_cast magic. memcpy first.
	std::uint64_t quo(0);
	for(std::size_t i(v.size());i--;)
	{
		auto const tot(v[i]+(quo<<32));
		v[i]=tot/value;
		quo=tot%value;
	}
	memcpy(static_cast<void*>(std::addressof(a)),v.data(),sizeof(v));
	return quo;
}

template<typename T>
inline constexpr basic_unsigned_extension<T> operator/(basic_unsigned_extension<T> a,basic_unsigned_extension<T> const& b)
{
	return div_mod(a,b).first;
}

template<typename T>
inline constexpr std::uint32_t operator%(basic_unsigned_extension<T> const& a,std::uint32_t value)
{
	std::array<std::uint32_t,sizeof(T)/2> v;
	memcpy(v.data(),std::addressof(a),sizeof(v));		//should use std::bit_cast instead. However, current compilers haven't implemented std::bit_cast magic. memcpy first.
	std::uint64_t quo(0);
	for(std::size_t i(v.size());i--;)
	{
		auto const tot(v[i]+(quo<<32));
		v[i]=tot/value;
		quo=tot%value;
	}
	return quo;
}

template<typename T>
inline constexpr basic_unsigned_extension<T> operator%(basic_unsigned_extension<T> a,basic_unsigned_extension<T> const& b)
{
	return div_mod(a,b).second;
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator/=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	return a=a/b;
}

template<typename T>
inline constexpr basic_unsigned_extension<T>& operator%=(basic_unsigned_extension<T>& a,basic_unsigned_extension<T> const& b)
{
	return a=a%b;
}

template<typename T>
inline constexpr auto pow(basic_unsigned_extension<T> lhs, basic_unsigned_extension<T> rhs)
{
	basic_unsigned_extension<T> ans(1);
	while(rhs)
	{
		if(rhs.front())
			ans *= lhs;
		rhs >>= 1;
		lhs *=lhs;
	}
	return ans;
}

template<typename T,typename P>
requires
#ifdef __SIZEOF_INT128__
(std::unsigned_integral<P>||std::same_as<P,__uint128_t>)
#else
(std::unsigned_integral<P>)
#endif
inline constexpr auto pow(basic_unsigned_extension<T> lhs, P rhs)
{
	basic_unsigned_extension<T> ans(1);
	while(rhs)
	{
		if(rhs&1)
			ans *= lhs;
		rhs >>= 1;
		lhs *=lhs;
	}
	return ans;
}

namespace details
{

template<std::uint8_t base,bool uppercase,character_output_stream output,typename T>
inline void output_base_extension_number(output& out,basic_unsigned_extension<T> a)
{
//number: 0:48 9:57
//upper: 65 :A 70: F
//lower: 97 :a 102 :f
	if(!a)
	{
		put(out,0x30);
		return;
	}
	std::array<typename output::char_type,sizeof(a)*512*8/base+3> v;
	auto iter(v.data()+v.size());
	while(a)
	{
		std::uint32_t rem(in_place_div_mod(a,base));
		if constexpr(10 < base)
		{
			if(rem<10)
				*--iter = static_cast<typename output::char_type>(rem+48);
			else
			{
				if constexpr (uppercase)
					*--iter = static_cast<typename output::char_type>(rem+55);	
				else
					*--iter = static_cast<typename output::char_type>(rem+87);
			}
		}
		else
			*--iter = static_cast<typename output::char_type>(rem+48);
	}
	send(out,iter,v.data()+v.size());
}
}

template<output_stream output,typename T>
inline constexpr void print_define(output& out,basic_unsigned_extension<T> const& a)
{
	details::output_base_extension_number<10,false>(out,a);
}

template<std::size_t base,bool uppercase,character_output_stream output,typename T>
inline constexpr void print_define(output& out,manip::base_t<base,uppercase,basic_unsigned_extension<T> const> v)
{
	details::output_base_extension_number<base,uppercase>(out,v.reference);
}
template<std::size_t base,bool uppercase,character_output_stream output,typename T>
inline constexpr void print_define(output& out,manip::base_t<base,uppercase,basic_unsigned_extension<T>> v)
{
	details::output_base_extension_number<base,uppercase>(out,v.reference);
}

namespace details
{
template<std::uint8_t base,character_input_stream input,typename T>
inline constexpr void input_base_number_phase2_extension(input& in,basic_unsigned_extension<T>& a)
{
	using unsigned_char_type = std::make_unsigned_t<decltype(get(in))>;
	unsigned_char_type constexpr baseed(std::min(static_cast<unsigned_char_type>(base),static_cast<unsigned_char_type>(10)));
	while(true)
	{
		unsigned_char_type ch(get<true>(in).first);
		if((ch-=48)<baseed)
		{
			a*=base;
			a+=ch;
		}
		else if constexpr (10 < base)
		{
			unsigned_char_type constexpr bm10(base-10);
			if((ch-=17)<bm10||(ch-=32)<bm10)
				a=a*base+(ch+10);
			else
				return;
		}
		else
			return;
	}
}

template<std::uint8_t base,character_input_stream input,typename T>
inline constexpr void input_base_extension_number(input& in,basic_unsigned_extension<T>& a)
{
	using unsigned_char_type = std::make_unsigned_t<decltype(get(in))>;
	unsigned_char_type constexpr baseed(std::min(static_cast<unsigned_char_type>(base),static_cast<unsigned_char_type>(10)));
	while(true)
	{
		unsigned_char_type ch(get(in));
		if((ch-=48)<baseed)
		{
			a=static_cast<basic_unsigned_extension<T>>(ch);
			break;
		}
		else if constexpr (10 < base)
		{
			unsigned_char_type constexpr bm10(base-10);
			if((ch-=17)<bm10||(ch-=32)<bm10)
			{
				a=static_cast<basic_unsigned_extension<T>>(ch+10);
				break;
			}
		}
	}
	input_base_number_phase2_extension<base>(in,a);
}
}

template<std::size_t base,bool uppercase,character_input_stream input,typename T>
inline constexpr void scan_define(input& in,manip::base_t<base,uppercase,basic_unsigned_extension<T>> v)
{
	details::input_base_extension_number<base>(in,v.reference);
}

template<character_input_stream input,typename T>
inline constexpr void scan_define(input& in,basic_unsigned_extension<T>& a)
{
	details::input_base_extension_number<10>(in,a);
}

template<character_output_stream output,typename T>
inline constexpr void write_define(output& out,basic_unsigned_extension<T> const& n)
{
	send(out,std::addressof(n),std::addressof(n)+1);
}

template<character_input_stream input,typename T>
inline constexpr void read_define(input& in,basic_unsigned_extension<T>& n)
{
	receive(in,std::addressof(n),std::addressof(n)+1);
}
#ifdef __SIZEOF_INT128__
using uint128_t = __uint128_t;
#else
using uint128_t = basic_unsigned_extension<std::uint64_t>;
#endif
using uint256_t = basic_unsigned_extension<uint128_t>;
/*
using uint512_t = basic_unsigned_extension<uint256_t>;
using uint1024_t = basic_unsigned_extension<uint512_t>;
using uint2048_t = basic_unsigned_extension<uint1024_t>;
using uint4096_t = basic_unsigned_extension<uint2048_t>;

namespace literals
{

inline constexpr auto operator "" _u128(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint128_t nt;
	scan(is,nt);
	return nt;
}

inline constexpr auto operator "" _u256(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint256_t nt;
	scan(is,nt);
	return nt;
}

inline constexpr auto operator "" _u512(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint512_t nt;
	scan(is,nt);
	return nt;
}
inline constexpr auto operator "" _u1024(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint1024_t nt;
	scan(is,nt);
	return nt;
}
inline constexpr auto operator "" _u2048(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint2048_t nt;
	scan(is,nt);
	return nt;
}
inline constexpr auto operator "" _u4096(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint4096_t nt;
	scan(is,nt);
	return nt;
}

inline constexpr auto operator "" _u128b(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint128_t nt;
	scan(is,bin(nt));
	return nt;
}

inline constexpr auto operator "" _u256b(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint256_t nt;
	scan(is,bin(nt));
	return nt;
}

inline constexpr auto operator "" _u512b(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint512_t nt;
	scan(is,bin(nt));
	return nt;
}
inline constexpr auto operator "" _u1024b(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint1024_t nt;
	scan(is,bin(nt));
	return nt;
}
inline constexpr auto operator "" _u2048b(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint2048_t nt;
	scan(is,bin(nt));
	return nt;
}
inline constexpr auto operator "" _u4096b(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint4096_t nt;
	scan(is,bin(nt));
	return nt;
}


inline constexpr auto operator "" _u128o(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint128_t nt;
	scan(is,oct(nt));
	return nt;
}

inline constexpr auto operator "" _u256o(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint256_t nt;
	scan(is,oct(nt));
	return nt;
}

inline constexpr auto operator "" _u512o(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint512_t nt;
	scan(is,oct(nt));
	return nt;
}
inline constexpr auto operator "" _u1024o(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint1024_t nt;
	scan(is,oct(nt));
	return nt;
}
inline constexpr auto operator "" _u2048o(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint2048_t nt;
	scan(is,oct(nt));
	return nt;
}
inline constexpr auto operator "" _u4096o(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint4096_t nt;
	scan(is,oct(nt));
	return nt;
}


inline constexpr auto operator "" _u128h(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint128_t nt;
	scan(is,hex(nt));
	return nt;
}

inline constexpr auto operator "" _u256h(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint256_t nt;
	scan(is,hex(nt));
	return nt;
}

inline constexpr auto operator "" _u512h(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint512_t nt;
	scan(is,hex(nt));
	return nt;
}
inline constexpr auto operator "" _u1024h(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint1024_t nt;
	scan(is,hex(nt));
	return nt;
}
inline constexpr auto operator "" _u2048h(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint2048_t nt;
	scan(is,hex(nt));
	return nt;
}
inline constexpr auto operator "" _u4096h(char const* cstr, size_t n)
{
	istring_view is(cstr,n);
	uint4096_t nt;
	scan(is,hex(nt));
	return nt;
}

}*/

}