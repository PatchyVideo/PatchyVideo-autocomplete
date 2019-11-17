#pragma once

namespace fast_io::details::ryu
{

template<std::floating_point T>
struct floating_traits
{
};


template<>	
struct floating_traits<float>
{
	using mantissa_type = std::uint32_t;
	using exponent_type = std::uint32_t;
	static inline constexpr exponent_type exponent_bits = 8;
	static inline constexpr mantissa_type mantissa_bits = sizeof(float)*8-1-exponent_bits;
	static inline constexpr exponent_type exponent_max = (static_cast<exponent_type>(1)<<exponent_bits)-1;
	static inline constexpr exponent_type bias = (static_cast<exponent_type>(1)<<(exponent_bits - 1)) - 1;
//	static inline constexpr exponent_type pow5_inv_bitcount= 61;
	static inline constexpr exponent_type pow5_bitcount= 61;
	static inline constexpr exponent_type floor_log5 = 9;
	static inline constexpr exponent_type bound = 31;//ryu to do. use a tigher bound
};

template<>	
struct floating_traits<double>
{
	using mantissa_type = std::uint64_t;
	using exponent_type = std::uint32_t;
	static inline constexpr exponent_type exponent_bits = 11;
	static inline constexpr mantissa_type mantissa_bits = sizeof(double)*8-1-exponent_bits;
	static inline constexpr exponent_type exponent_max = (static_cast<exponent_type>(1)<<exponent_bits)-1;
	static inline constexpr exponent_type bias = (static_cast<exponent_type>(1)<<(exponent_bits - 1)) - 1;
//	static inline constexpr exponent_type pow5_inv_bitcount= 125;
	static inline constexpr exponent_type pow5_bitcount= 125;
	static inline constexpr exponent_type floor_log5 = 21;
	static inline constexpr exponent_type bound = 63;//ryu to do. use a tigher bound
};

template<>	
struct floating_traits<long double>
{
	using mantissa_type = uint128_t;
	using exponent_type = std::uint32_t;
	static inline constexpr exponent_type exponent_bits = 17;
	static inline constexpr std::uint32_t mantissa_bits = sizeof(long double)*8-1-exponent_bits;
	static inline constexpr exponent_type exponent_max = (static_cast<exponent_type>(1)<<exponent_bits)-1;
	static inline constexpr exponent_type bias = (static_cast<exponent_type>(1)<<(exponent_bits - 1)) - 1;
//	static inline constexpr std::size_t pow5_inv_bitcount= 249;
	static inline constexpr std::size_t pow5_bitcount= 249;
	static inline constexpr exponent_type floor_log5 = 55;
	static inline constexpr exponent_type bound = 127;//ryu to do. use a tigher bound
};

template<std::integral mantissaType,std::integral exponentType>
struct unrep
{
	using mantissa_type = mantissaType;
	using exponent_type = exponentType;
	mantissa_type m=0;
	exponent_type e=0;
};
template<std::unsigned_integral T>
inline constexpr T index_for_exponent(T e){return (e+15)>>4;}

template<std::unsigned_integral T>
inline constexpr T pow10_bits_for_index(T idx){return (idx<<4)+120;}

template<std::unsigned_integral T>
inline constexpr bool multiple_of_power_of_2(T value,std::size_t p) {
// return __builtin_ctz(value) >= p;
return !(static_cast<uint128_t>(value) & ((static_cast<uint128_t>(1)<<p) - 1));
}

template<typename T>
inline constexpr std::uint32_t pow5_factor(T value)
{
	for (std::uint32_t count(0);value;++count)
	{
		if (value%5)
			return count;
		value/=5;
	}
	return 0;
}

template<std::integral U>
inline constexpr std::int32_t pow5bits(U e)
{
	return static_cast<int32_t>(((static_cast<std::uint32_t>(e) * 1217359) >> 19) + 1);
}


// Returns true if value is divisible by 5^p.
template<typename T>
inline constexpr bool multiple_of_power_of_5(T value,std::uint32_t p)
{
	// The author tried a case distinction on p, but there was no performance difference.
	return p<=pow5_factor(value);
}

inline constexpr uint32_t log10_pow2(std::uint64_t e)
{
	return static_cast<std::uint32_t> (((static_cast<std::uint64_t>(e)) * 169464822037455ull) >> 49);
}
template<std::unsigned_integral T>
inline constexpr std::size_t length_for_index(T idx){return (log10_pow2(idx<<4)+25)/9;}

template<std::integral T>
inline constexpr uint32_t log10_pow5(T e)
{
	// The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
	return static_cast<uint32_t> (((static_cast<uint64_t>(e)) * 196742565691928ull) >> 48);
}
/*
template<bool controller,std::unsigned_integral T>
inline constexpr std::array<fast_io::uint128_t,2> compute_pow5(T v)
{
	std::uint32_t const base(v/56);
	std::uint32_t const base2(base*56);
	std::array<std::uint64_t,4> const& mul(pow5<long double,controller>::split[base]);
	if(v==base2)
		return {construct_unsigned_extension(mul.front(),mul[1]),construct_unsigned_extension(mul[2],mul[3])};
	else
	{
		std::uint32_t const offset(v - base2);
		std::array<std::uint64_t,2> const &m = pow5<long double,controller>::[offset];
		const uint32_t delta = pow5bits(i) - pow5bits(base2);
		const uint32_t corr = (uint32_t) ((POW5_ERRORS[i / 32] >> (2 * (i % 32))) & 3);
		mul_128_256_shift(m, mul, delta, corr, result);
	}
//		return pow5<long double,controller>::;
//	pow5<long double,true>::inv_split[q]
}

template<std::unsigned_integral T>
inline constexpr std::array<fast_io::uint128_t,2> compute_pow5_inv(T v)
{
}
*/
template<std::unsigned_integral T,std::size_t muldiff=sizeof(T)*8>
requires std::same_as<T,std::uint64_t>||std::same_as<T,fast_io::uint128_t>
inline constexpr T mul_shift(T m, std::array<T,2> const& mul, std::size_t j)
{
	return low((mul_extend(m,mul.back())+high(mul_extend(m,mul.front())))>>(j-muldiff));
}

template<typename T>
requires (std::same_as<std::uint64_t,T>||std::same_as<fast_io::uint128_t,T>)
inline constexpr std::array<T,3> mul_shift_all(T m, std::array<T,2> const& mul,std::size_t j,std::uint32_t mmshift)
{
	auto const m4(m<<2);
	return {mul_shift(m4,mul,j),mul_shift(m4+2,mul,j),mul_shift(m4-1-mmshift,mul,j)};
}

template<typename T>
inline constexpr std::uint32_t mul_shift_mod_1e9(std::uint64_t m, std::array<T,3> const& mul, std::size_t j)
{
	uint128_t b1(mul_extend(m,mul[1]));
	b1+=high(mul_extend(m,mul[0]));
	uint128_t s1(mul_extend(m,mul[2]));
	s1+=high(b1);
	s1>>=j-128;
	uint128_t constexpr mulb(construct_unsigned_extension(static_cast<std::uint64_t>(0x31680A88F8953031),static_cast<std::uint64_t>(0x89705F4136B4A597)));
	return static_cast<std::uint32_t>(s1)-1000000000*static_cast<std::uint32_t>(low(mul_high(s1,mulb))>>29);
}

template<std::random_access_iterator Iter,std::unsigned_integral mantissaType>
inline constexpr auto easy_case(Iter result,bool sign, mantissaType const& mantissa)
{
	if (mantissa)
		return std::copy_n("nan",3,result);
	if (sign)
		return std::copy_n("-inf",4,result);
	return std::copy_n("inf",3,result);
}

template<std::floating_point floating,std::unsigned_integral mantissaType,std::signed_integral exponentType>
inline constexpr unrep<mantissaType,exponentType> init_rep(mantissaType const& mantissa,exponentType const& exponent)
{
	if(!exponent)
		return {mantissa,1-static_cast<exponentType>(floating_traits<floating>::bias+floating_traits<floating>::exponent_bits)};
	return {static_cast<mantissaType>((static_cast<mantissaType>(1)<<floating_traits<floating>::mantissa_bits)|mantissa),
		static_cast<exponentType>(exponent-static_cast<exponentType>(floating_traits<floating>::bias+floating_traits<floating>::mantissa_bits))};
}

template<bool uppercase_e=false,std::signed_integral T,std::random_access_iterator Iter>
requires std::same_as<T,std::int32_t>
inline constexpr Iter output_exp(T exp,Iter result)
{
	if constexpr(uppercase_e)
		*result='E';
	else
		*result='e';
	++result;
	if(exp<0)
	{
		*result='-';
		++result;
		exp=-exp;
	}
	else
	{
		*result='+';
		++result;
	}
	using char_type = std::remove_reference_t<decltype(*result)>;
	std::make_unsigned_t<T> unsigned_exp(exp);
	if(99<unsigned_exp)
	{
		auto const quo(unsigned_exp/100);
		unsigned_exp%=100;
		*result=static_cast<char_type>(quo+'0');
		++result;
	}
	auto exp_tb(shared_static_base_table<10,false>::table[unsigned_exp]);
	constexpr auto sz(exp_tb.size()-2);
	return std::copy_n(exp_tb.data()+sz,2,result);
}

template<std::size_t precision,bool scientific = false,bool uppercase_e=false,std::random_access_iterator Iter,std::floating_point F>
inline constexpr auto output_fixed(Iter result, F d)
{
	using floating_trait = floating_traits<F>;
	using mantissa_type = typename floating_trait::mantissa_type;
	using exponent_type = typename floating_trait::exponent_type;
	using signed_exponent_type = std::make_signed_t<exponent_type>;
	using char_type = std::remove_reference_t<decltype(*result)>;
	auto const bits(bit_cast<mantissa_type>(d));
	// Decode bits into sign, mantissa, and exponent.
	bool const sign((bits >> (floating_trait::mantissa_bits + floating_trait::exponent_bits)) & 1u);
	mantissa_type const mantissa(bits & ((static_cast<mantissa_type>(1u) << floating_trait::mantissa_bits) - 1u));
	exponent_type const exponent(static_cast<exponent_type>(((bits >> floating_trait::mantissa_bits) & floating_trait::exponent_max)));
	// Case distinction; exit early for the easy cases.
	if(exponent == floating_trait::exponent_max)
		return easy_case(result,sign,mantissa);
	auto start(result);
	if(!exponent&&!mantissa)
	{
		if(sign)
		{
			*result='-';
			++result;
		}
		*result='0';
		++result;
		if constexpr(precision!=0)
		{
			*result='.';
			++result;
			result=std::fill_n(result,precision,'0');
			if constexpr(scientific)
			{
				if constexpr(uppercase_e)
					return std::copy_n("E+00",4,result);
				else
					return std::copy_n("e+00",4,result);
			}
		}
		return result;
	}
	auto const r2(init_rep<F>(mantissa,static_cast<signed_exponent_type>(exponent)));
	if (sign)
	{
		*result='-';
		++result;
	}
	bool const negative_r2_e(r2.e<0);
	if constexpr(scientific)
	{
		constexpr std::size_t scientific_precision(precision+1);
		exponent_type digits(0),printed_digits(0),available_digits(0);
		signed_exponent_type exp(0);
		if(-52<=r2.e)
		{
			exponent_type const idx(negative_r2_e?0:index_for_exponent(static_cast<exponent_type>(r2.e)));
			signed_exponent_type const p10bitsmr2e(pow10_bits_for_index(idx)-r2.e+8);
			auto const idx_offset(fixed_pow10<>::offset[idx]);
			for(std::size_t i(length_for_index(idx));i--;)
			{
				digits=mul_shift_mod_1e9(r2.m<<8,fixed_pow10<>::split[idx_offset+i],p10bitsmr2e);
				if(printed_digits)
				{
					if constexpr(precision<9)
					{
						available_digits=9;
						break;
					}
					else if(scientific_precision < printed_digits + 9)
					{
						available_digits=9;
						break;
					}
					std::fill(result,output_base_number_impl<10,false>(result+9,digits),'0');
					result+=9;
					printed_digits+=9;
				}
				else if(digits)
				{
					available_digits = chars_len<10,true>(digits);
					exp = static_cast<signed_exponent_type>(i*9 + available_digits - 1);
					if(scientific_precision < available_digits)
						break;
					if constexpr (precision!=0)
						output_base_number_impl<10,false,true>(result+=available_digits+1,digits);
					else
					{
						*result=static_cast<char_type>('0'+digits);
						++result;
					}
					printed_digits = available_digits;
					available_digits = 0;
				}
			}
		}
		if(negative_r2_e&&!available_digits)
		{
			auto abs_e2(-r2.e);
			exponent_type const idx(static_cast<exponent_type>(abs_e2)>>4);
			signed_exponent_type j(128+(abs_e2-(idx<<4)));
			exponent_type const of2i(fixed_pow10<>::offset_2[idx]);
			exponent_type const idxp1(fixed_pow10<>::offset_2[idx+1]);
			exponent_type const mb2_idx(fixed_pow10<>::min_block_2[idx]);
			for (exponent_type i(mb2_idx); i < 200; ++i)
			{
				exponent_type const p(of2i+i-mb2_idx);
				digits=(idxp1<=p)?0:mul_shift_mod_1e9(r2.m<<8,fixed_pow10<>::split_2[p],j);
				if(printed_digits)
				{
					if constexpr(precision<9)
					{
						available_digits=9;
						break;
					}
					else if(scientific_precision < printed_digits + 9)
					{
						available_digits=9;
						break;
					}
					std::fill(result,output_base_number_impl<10,false>(result+9,digits),'0');
					result+=9;
					printed_digits+=9;
				}
				else if(digits)
				{
					available_digits=chars_len<10,true>(digits);
					exp = static_cast<signed_exponent_type> (available_digits -(i + 1) * 9 - 1);
					if (scientific_precision<available_digits)
						break;
					if constexpr (precision!=0)
						output_base_number_impl<10,false,true>(result+=available_digits+1,digits);
					else
					{
						*result=static_cast<char_type>('0'+digits);
						++result;
					}
					printed_digits = available_digits;
					available_digits = 0;
				}
			}
		}
		exponent_type const maximum(scientific_precision - printed_digits);
		exponent_type lastdigit(0);
		for(exponent_type k(maximum);k<available_digits;++k)
		{
			lastdigit = digits%10;
			digits /= 10;
		}
		std::size_t round_up(0);
		if(lastdigit!=5)
			round_up = 5 < lastdigit;
		else
		{
			signed_exponent_type const rexp (static_cast<signed_exponent_type> (scientific_precision - exp));
			signed_exponent_type const required_twos(-r2.e - rexp);
			bool trailing_zeros(required_twos <= 0 || (required_twos < 60 && multiple_of_power_of_2(r2.m, static_cast<exponent_type>(required_twos))));
			if (rexp < 0)
			{
				signed_exponent_type required_fives = -rexp;
				trailing_zeros = trailing_zeros && multiple_of_power_of_5(r2.m, static_cast<exponent_type>(required_fives));
			}
			round_up = trailing_zeros ? 2 : 1;
		}
		if(printed_digits)
		{
			if(digits)
			{
				std::fill(result,output_base_number_impl<10,false>(result+maximum,digits),'0');
				result+=maximum;
			}
			else
				result=std::fill_n(result,maximum,'0');
		}
		else
		{
			if constexpr(precision!=0)
			{
				std::fill(result,output_base_number_impl<10,false,true>(result+maximum+1,digits),'0');
				result+=maximum+1;
			}
			else
			{
				*result = '0' + digits;
				++result;
			}
		}
		if(round_up)
		{
			std::size_t round_index(result-start);
			while(round_index--)
			{
				auto c(start[round_index]);
				if (c == '-')
				{
					start[round_index+1] = '1';
					++exp;
					break;
				}
				if constexpr(precision==0)
				{
					if (c == '9')
					{
						start[round_index] = '0';
						round_up = 1;
						continue;
					}
					else
					{
						if (round_up==2&&!(c&1))
							break;
						start[round_index]=c+1;
						break;
					}
				}
				else
				{
					if (c == '.')
						continue;
					else if (c == '9')
					{
						start[round_index] = '0';
						round_up = 1;
						continue;
					}
					else
					{
						if (round_up==2&&!(c&1))
							break;
						start[round_index]=c+1;
						break;
					}
				}
			}
			if(round_index==static_cast<std::size_t>(-1))
			{
				start[round_index+1] = '1';
				++exp;
			}
		}
		return output_exp<uppercase_e>(exp,result);
	}
	else
	{
		bool nonzero(false);
		if(-52<=r2.e)
		{
			exponent_type const idx(negative_r2_e?0:index_for_exponent(static_cast<exponent_type>(r2.e)));
			signed_exponent_type const p10bitsmr2e(pow10_bits_for_index(idx)-r2.e+8);
			for(std::size_t i(length_for_index(idx));i--;)
			{
				exponent_type digits(mul_shift_mod_1e9(r2.m<<8,fixed_pow10<>::split[fixed_pow10<>::offset[idx]+i],p10bitsmr2e));
				if(nonzero)
				{
					std::fill(result,output_base_number_impl<10,false>(result+9,digits),'0');
					result+=9;
				}
				else if(digits)
				{
					output_base_number_impl<10,false>(result+=chars_len<10,true>(digits),digits);
					nonzero = true;
				}
			}
		}
		if(!nonzero)
		{
			*result='0';
			++result;
		}
		if constexpr(precision!=0)
		{
			*result='.';
			++result;
		}
		if(negative_r2_e)
		{
			auto abs_e2(-r2.e);
			exponent_type const idx(static_cast<exponent_type>(abs_e2)>>4);
			constexpr std::size_t blocks(precision/9+1);
			std::size_t round_up(0);
			std::size_t i(0);
			auto const mb2_idx(fixed_pow10<>::min_block_2[idx]);
			if (blocks<=mb2_idx)
			{
				i=blocks;
				result=std::fill_n(result,precision,'0');
			}
			else if(i<mb2_idx)
				result=std::fill_n(result,9*(i=mb2_idx),'0');
			signed_exponent_type j(128+(abs_e2-(idx<<4)));
			auto const of2i(fixed_pow10<>::offset_2[idx]);
			for(;i<blocks;++i)
			{
				exponent_type p(of2i+i-mb2_idx);
				exponent_type digits(mul_shift_mod_1e9(r2.m<<8,fixed_pow10<>::split_2[p],j));
				if (fixed_pow10<>::offset_2[idx+1]<=p)
				{
					result=std::fill_n(result,precision-9*i,'0');
					break;
				}
				if(i+1<blocks)
				{
					std::fill(result,output_base_number_impl<10,false>(result+9,digits),'0');
					result+=9;
				}
				else
				{
					exponent_type const maximum(precision-9*i);
					exponent_type lastdigit(0);
					for(exponent_type k(maximum);k<9;++k)
					{
						lastdigit = digits%10;
						digits /= 10;
					}
					if(lastdigit!=5)
						round_up=lastdigit>5;
					else
					{
						auto const required_twos(static_cast<signed_exponent_type>(abs_e2-precision-1));
						if(required_twos<=0||(required_twos<60&&multiple_of_power_of_2(r2.m,static_cast<exponent_type>(required_twos))))
							round_up = 2;
						else
							round_up = 1;
					}
					if(maximum)
					{
						std::fill(result,output_base_number_impl<10,false>(result+maximum,digits),'0');
						result+=maximum;
					}
					break;
				}
			}
			if(round_up)
			{
				std::size_t round_index(result-start);
				if constexpr(precision!=0)
				{
					std::size_t dot_index(0);
					while(round_index--)
					{
						auto c(start[round_index]);
						if (c == '-')
						{
							start[round_index+1] = '1';
							if(dot_index)
							{
								start[dot_index] = '0';
								start[dot_index+1] = '.';
							}
							*result='0';
							return ++result;
						}
						if (c == '.')
						{
							dot_index = round_index;
							continue;
						}
						else if (c == '9')
						{
							start[round_index] = '0';
							round_up = 1;
							continue;
						}
						if (round_up!=2||c&1)
							start[round_index]=c+1;
						return result;
					}
					*start='1';
					if(dot_index)
					{
						start[dot_index] = '0';
						start[dot_index+1] = '.';
					}
				}
				else
				{
					while(round_index--)
					{
						auto c(start[round_index]);
						if (c == '-')
						{
							start[round_index+1] = '1';
							*result='0';
							return ++result;
						}
						if (c == '9')
						{
							start[round_index] = '0';
							round_up = 1;
							continue;
						}
						if (round_up!=2||c&1)
							start[round_index]=c+1;
						return result;
					}
					*start='1';
				}
				*result='0';
				++result;
			}
			return result;
		}
		return std::fill_n(result,precision,'0');
	}
}
template<std::floating_point floating,std::unsigned_integral mantissaType,std::signed_integral exponentType>
inline constexpr unrep<mantissaType,exponentType> init_repm2(mantissaType const& mantissa,exponentType const& exponent)
{
	if(!exponent)
		return {mantissa,1-static_cast<exponentType>(floating_traits<floating>::bias+floating_traits<floating>::exponent_bits+2)};
	return {static_cast<mantissaType>((static_cast<mantissaType>(1)<<floating_traits<floating>::mantissa_bits)|mantissa),
		static_cast<exponentType>(exponent-static_cast<exponentType>(floating_traits<floating>::bias+floating_traits<floating>::mantissa_bits+2))};
}

template<bool uppercase_e=false,std::size_t mode=0,std::random_access_iterator Iter,std::floating_point F>
inline constexpr Iter output_shortest(Iter result, F d)
{
	using floating_trait = floating_traits<F>;
	using mantissa_type = typename floating_trait::mantissa_type;
	using exponent_type = typename floating_trait::exponent_type;
	using signed_exponent_type = std::make_signed_t<exponent_type>;
	auto const bits(bit_cast<mantissa_type>(d));
	// Decode bits into sign, mantissa, and exponent.
	bool const sign((bits >> (floating_trait::mantissa_bits + floating_trait::exponent_bits)) & 1u);
	mantissa_type const mantissa(bits & ((static_cast<mantissa_type>(1u) << floating_trait::mantissa_bits) - 1u));
	exponent_type const exponent(static_cast<exponent_type>(((bits >> floating_trait::mantissa_bits) & floating_trait::exponent_max)));
	// Case distinction; exit early for the easy cases.
	if(exponent == floating_trait::exponent_max)
		return easy_case(result,sign,mantissa);
	if(!exponent&&!mantissa)
	{
		if(sign)
		{
			*result='-';
			++result;
		}
		*result='0';
		++result;
		if constexpr(mode==2)
		{
			if constexpr(uppercase_e)
				return std::copy_n("E+00",4,result);
			else
				return std::copy_n("e+00",4,result);
		}
		return result;
	}
	auto const r2(init_repm2<F>(mantissa,static_cast<signed_exponent_type>(exponent)));
	bool const accept_bounds(!(r2.m&1));
	auto const mv(r2.m<<2);
	exponent_type const mm_shift(mantissa||static_cast<signed_exponent_type>(exponent)<2);
	std::array<mantissa_type,3> v{};
	//vr,vp,vm
	signed_exponent_type e10{};
	bool vm_is_trailing_zeros(false),vr_is_trailing_zeros(false);
	if(0<=r2.e)
	{
		exponent_type const q(log10_pow2(r2.e)-(3<r2.e));
		e10=static_cast<signed_exponent_type>(q);
		signed_exponent_type const k(floating_trait::pow5_bitcount + pow5bits(q) - 1);
		signed_exponent_type const i(-r2.e+static_cast<signed_exponent_type>(q)+k);
		if constexpr(std::same_as<std::remove_cvref_t<F>,long double>)
			v=mul_shift_all(r2.m,compute_pow5_inv(q),i,mm_shift);
		else
			v=mul_shift_all(r2.m,pow5<F,true>::inv_split[q],i,mm_shift);
		if(q<=floating_trait::floor_log5)//here
		{
			if(!(mv%5))
				vm_is_trailing_zeros=multiple_of_power_of_5(mv,q);
			else if(accept_bounds)
				vm_is_trailing_zeros=multiple_of_power_of_5(mv-1-mm_shift,q);
			else
				v[1]-=multiple_of_power_of_5(mv+2,q);
		}
	}
	else
	{
		exponent_type abs_e2(static_cast<exponent_type>(-r2.e));
		exponent_type const q(log10_pow5(abs_e2)-(1<abs_e2));
		signed_exponent_type const signed_q(static_cast<signed_exponent_type>(q));
		e10=signed_q+r2.e;
		signed_exponent_type const i(-r2.e-signed_q);
		signed_exponent_type const k(pow5bits(i)-floating_trait::pow5_bitcount);
		signed_exponent_type const j(signed_q-k);
		if constexpr(std::same_as<std::remove_cvref_t<F>,long double>)
			v=mul_shift_all(r2.m,compute_pow5(i),j,mm_shift);
		else
			v=mul_shift_all(r2.m,pow5<F,true>::split[i],j,mm_shift);
		if(q<2)
		{
			vr_is_trailing_zeros=true;
			if(accept_bounds)
				vm_is_trailing_zeros=mm_shift==1;
			else
				--v[1];
		}
		else if(q<floating_trait::bound)
			vr_is_trailing_zeros=multiple_of_power_of_2(mv,q);
	}
	signed_exponent_type removed(0);
	std::uint8_t last_removed_digit(0);
	if(vm_is_trailing_zeros||vr_is_trailing_zeros)
	{
		for(;;)
		{
			mantissa_type const vpdiv10(v[1]/10);
			mantissa_type const vmdiv10(v[2]/10);
			auto const vmmod10(static_cast<std::uint8_t>(v[2]%10));
			if(vpdiv10 <= vmdiv10)
				break;
			mantissa_type const vrdiv10(v.front()/10);
			auto const vrmod10(static_cast<std::uint8_t>(v.front()%10));
			vm_is_trailing_zeros&=!vmmod10;
			vr_is_trailing_zeros&=!last_removed_digit;
			last_removed_digit=static_cast<std::uint8_t>(vrmod10);
			v.front()=vrdiv10;
			v[1]=vpdiv10;
			v[2]=vmdiv10;
			++removed;
		}
		if(vm_is_trailing_zeros)
			for(;;)
			{
				mantissa_type const vmdiv10(v[2]/10);
				auto const vmmod10(static_cast<std::uint8_t>(v[2]%10));
				if(vmmod10)
					break;
				mantissa_type const vpdiv10(v[1]/10);
				mantissa_type const vrdiv10(v.front()/10);
				auto const vrmod10(v.front()%10);
				vr_is_trailing_zeros&=!last_removed_digit;
				last_removed_digit=static_cast<std::uint8_t>(vrmod10);
				v.front()=vrdiv10;
				v[1]=vpdiv10;
				v[2]=vmdiv10;
				++removed;
			}
		if(vr_is_trailing_zeros&&last_removed_digit==5&&!(v.front()&1))
			last_removed_digit=4;
		v.front() += ((v.front()==std::get<2>(v)&&(!accept_bounds || !vm_is_trailing_zeros))|| 4 < last_removed_digit);
	}
	else
	{
		bool round_up(false);
		mantissa_type const vpdiv100(v[1]/100);
		mantissa_type const vmdiv100(v[2]/100);
		if(vmdiv100<vpdiv100)
		{
			mantissa_type const vrdiv100(v.front()/100);
			auto const vrmod100(v.front()%100);
			round_up=50<=vrmod100;
			v.front()=vrdiv100;
			v[1]=vpdiv100;
			v[2]=vmdiv100;
			removed+=2;
		}
		for (;;)
		{
			mantissa_type const vpdiv10(v[1]/10);
			mantissa_type const vmdiv10(v[2]/10);
			if(vpdiv10<=vmdiv10)
				break;
			mantissa_type const vrdiv10(v.front()/10);
			auto const vrmod10(v.front()%10);
			round_up=5<=vrmod10;
			v.front()=vrdiv10;
			v[1]=vpdiv10;
			v[2]=vmdiv10;
			++removed;
		}
		v.front()+=(v.front()==v[2]||round_up);
	}
	if(sign)
	{
		*result='-';
		++result;
	}
	std::int32_t olength(static_cast<std::int32_t>(chars_len<10,true>(v.front())));
	std::int32_t const real_exp(static_cast<std::int32_t>(e10 + removed + olength - 1));
	if constexpr(mode==0) //shortest
	{
		std::uint32_t fixed_length(0),this_case(0);
		if(olength<real_exp)
		{
			fixed_length=real_exp+1;
			this_case=1;
		}
		else if(0<=real_exp&&real_exp<olength)
		{
			fixed_length=olength+2;
			if(olength==real_exp+1)
				--fixed_length;
			this_case=2;
		}
		else
			fixed_length=static_cast<exponent_type>(-real_exp)+olength+1;
		std::uint32_t scientific_length(olength==1?olength+3:olength+5);
		if(scientific_length<fixed_length)
		{
			if(olength==1)
				output_base_number_impl<10,false,false>(result+=olength,v.front());
			else
				output_base_number_impl<10,false,true>(result+=olength+1,v.front());
			return output_exp<uppercase_e>(static_cast<std::int32_t>(real_exp),result);
		}
		switch(this_case)
		{
		case 1:
			output_base_number_impl<10,false>(result+=olength,v.front());
			return std::fill_n(result,real_exp+1-olength,'0');
		case 2:
		{
			constexpr auto &table(details::shared_static_base_table<10,uppercase_e>::table);
			constexpr std::uint32_t pw(table.size());
			constexpr std::uint32_t chars(table.front().size());
			auto a(v.front());
			auto eposition(result+real_exp+1);
			auto iter(result+=olength);
			if(eposition!=iter)
			{
				++result;
				++iter;
				for(;eposition+2<iter&&pw<=a;)
				{
					auto const rem(a%pw);
					a/=pw;
					std::copy_n(table[rem].data(),chars,iter-=chars);
				}
				if(iter==eposition+2)
				{
					auto const rem(a%10);
					a/=10;
					*--iter=static_cast<char>('0'+rem);
				}
				*--iter='.';
			}
			output_base_number_impl<10,false>(iter,a);
			return result;
		}
		default:
			result=std::copy_n("0.",2,result);
			result=std::fill_n(result,static_cast<exponent_type>(-real_exp-1),'0');
			output_base_number_impl<10,false>(result+=olength,v.front());
			return result;
		}
	}
	else if constexpr(mode==1) //fixed
	{
		if(olength<real_exp)
		{
			output_base_number_impl<10,false>(result+=olength,v.front());
			return std::fill_n(result,real_exp+1-olength,'0');	
		}
		else if(0<=real_exp&&real_exp<olength)
		{
			constexpr auto &table(details::shared_static_base_table<10,uppercase_e>::table);
			constexpr std::uint32_t pw(table.size());
			constexpr std::uint32_t chars(table.front().size());
			auto a(v.front());
			auto eposition(result+real_exp+1);
			auto iter(result+=olength);
			if(eposition!=iter)
			{
				++result;
				++iter;
				for(;eposition+2<iter&&pw<=a;)
				{
					auto const rem(a%pw);
					a/=pw;
					std::copy_n(table[rem].data(),chars,iter-=chars);
				}
				if(iter==eposition+2)
				{
					auto const rem(a%10);
					a/=10;
					*--iter=static_cast<char>('0'+rem);
				}
				*--iter='.';
			}
			output_base_number_impl<10,false>(iter,a);
			return result;
		}
		else
		{
			result=std::copy_n("0.",2,result);
			result=std::fill_n(result,static_cast<exponent_type>(-real_exp-1),'0');
			output_base_number_impl<10,false>(result+=olength,v.front());
			return result;
		}
	}
	else		//scientific
	{
		if(olength==1)
			output_base_number_impl<10,false,false>(result+=olength,v.front());
		else
			output_base_number_impl<10,false,true>(result+=olength+1,v.front());
		return output_exp<uppercase_e>(static_cast<std::int32_t>(real_exp),result);
	}
}

}
