#pragma once

namespace fast_io::details::ryu
{

//FUCK CONSTEXPR NOT ALLOWING ME TO USE GOTO. FUCK WG21
template<std::floating_point F,character_input_stream input>
inline constexpr F input_floating(input& in)
{
	using floating_trait = floating_traits<F>;
	using mantissa_type = typename floating_trait::mantissa_type;
	using exponent_type = typename floating_trait::exponent_type;
	using unsigned_char_type = std::make_unsigned_t<std::remove_reference_t<decltype(get(in))>>;
	using signed_exponent_type = std::make_signed_t<exponent_type>;
	mantissa_type ipart{};
	bool negative{};
	std::uint8_t phase2{};
	//.(46)-48: static_cast<unsigned_char_type>(-2)
	//-(45)-48: static_cast<unsigned_char_type>(-3)
	//'E'(69)-48: 21
	//'e'(101)-48: 53
	//We do not support Shit like EBCDIC. DEATH TO IBM
	exponent_type m10digits{};
	signed_exponent_type m10e(0);
	bool zero_front{};
	for(;;)
	{
		unsigned_char_type ch(static_cast<unsigned_char_type>(get(in))-48);
		if(ch<10)
		{
			if(!ch)
			{
				zero_front=true;
				break;
			}
			ipart=ch;
			++m10digits;
			break;
		}
		else
		{
			
			if(ch==static_cast<unsigned_char_type>(-3))
			{
				if((ch=static_cast<unsigned_char_type>(get(in))-48)<10)
				{
					negative=true;
					if(!ch)
					{
						zero_front=true;
						break;
					}
					ipart=ch;
					++m10digits;
					break;
				}
				else if(ch==static_cast<unsigned_char_type>(-2))
				{
					negative=true;
					phase2=1;
					ipart={};
					break;
				}
			}
			else if(ch==static_cast<unsigned_char_type>(-2))
			{
				if((ch=static_cast<unsigned_char_type>(get(in))-48)<10)
				{
					--m10e;
					if(!ch)
					{
						zero_front=false;
						for(;;--m10e)
						{
							unsigned_char_type ch(static_cast<unsigned_char_type>(get(in))-48);
							if(ch==0)
								continue;
							else if(ch<10)
							{
								ipart=ch;
								--m10e;
								phase2 = 1;
								++m10digits;
								break;
							}
							else if(ch==21||ch==53)
							{
								phase2 = 0;
								break;
							}
							else
							{
								phase2 = 2;
								break;
							}
						}
						break;
					}
					phase2=1;
					ipart=ch;
					break;
				}
			}
		}
	}
	if(zero_front)
	{
		for(;;)
		{
			unsigned_char_type ch(static_cast<unsigned_char_type>(get(in))-48);
			if(ch==0)
				continue;
			else if(ch<10)
			{
				ipart=ch;
				++m10digits;
				break;
			}
			else if(ch==static_cast<unsigned_char_type>(-2))
			{
				for(;;--m10e)
				{
					unsigned_char_type ch(static_cast<unsigned_char_type>(get(in))-48);
					if(ch==0)
						continue;
					else if(ch<10)
					{
						ipart=ch;
						--m10e;
						phase2 = 1;
						++m10digits;
						break;
					}
					else if(ch==21||ch==53)
					{
						phase2 = 0;
						break;
					}
					else
					{
						phase2 = 2;
						break;
					}
				}
				break;
			}
			else if(ch==21||ch==53)
			{
				phase2 = 0;
				break;
			}
			else
			{
				phase2 = 2;
				break;
			}
		}
	}
	if(!phase2)
	{
		for(;m10digits<floating_trait::digits10;++m10digits)
		{
			unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
			if(ch<10)
				ipart=ipart*10+ch;
			else if(ch==static_cast<unsigned_char_type>(-2))
			{
				phase2 = 1;
				break;
			}
			else if(ch==21||ch==53)
			{
				phase2 = 0;
				break;
			}
			else
			{
				if(negative)
					return -static_cast<F>(ipart);
				return static_cast<F>(ipart);
			}
		}
		if(m10digits==floating_trait::digits10)
		{
			unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
//rounding. 4 discard. 6 carry. 5 is complex
			if(ch==5)
			{
				if(ipart&1)
				{
					if(++ipart==floating_trait::carry10)
					{
						ipart=floating_trait::carry10/10;
						++m10e;
					}
					for(;;++m10e)
					{
						unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
						if(ch==static_cast<unsigned_char_type>(-2))
						{
							phase2 = 1;
							break;
						}
						else if(ch==21||ch==53)
						{
							phase2 = 0;
							break;
						}
						else if(9<ch)
						{
							phase2 = 2;
							break;
						}
					}
				}
				else
				{
					for(;;++m10e)
					{
						unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
						if(ch==static_cast<unsigned_char_type>(-2))
						{
							phase2 = 1;
							break;
						}
						else if(ch==21||ch==53)
						{
							phase2 = 0;
							break;
						}
						else if(ch)
						{
							if(9<ch)
							{
								phase2 = 2;
								break;
							}
							++m10e;
							if(++ipart==floating_trait::carry10)
							{
								ipart=floating_trait::carry10/10;
								++m10e;
							}
							for(;;++m10e)
							{
								unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
								if(ch==static_cast<unsigned_char_type>(-2))
								{
									phase2 = 1;
									break;
								}
								else if(ch==21||ch==53)
								{
									phase2 = 0;
									break;
								}
								else if(9<ch)
									break;
							}
							break;
						}
					}
				}
			}
			if(ch<10)
			{
				++m10e;
				if(5<ch)
				{
					if(++ipart==floating_trait::carry10)
					{
						ipart=floating_trait::carry10/10;
						++m10e;
					}
				}
				for(;;++m10e)
				{
					unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
					if(ch==static_cast<unsigned_char_type>(-2))
					{
						phase2 = 1;
						break;
					}
					else if(ch==21||ch==53)
					{
						phase2 = 0;
						break;
					}
					else if(9<ch)
					{
						phase2 = 2;
						break;
					}
				}
			}
			else if(ch==static_cast<unsigned_char_type>(-2))
				phase2 = 1;
			else if(ch==21||ch==53)
				phase2 = 0;
			else
				phase2 = 2;
		}
	}
	if(phase2==1)
	{
		for(;m10digits<floating_trait::digits10;++m10digits,--m10e)
		{
			unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
			if(ch<10)
				ipart=ipart*10+ch;
			else if(ch==21||ch==53)
			{
				phase2 = 0;
				break;
			}
			else
			{
				phase2 = 2;
				break;
			}
		}

		//FUCK CONSTEXPR NOT ALLOWING ME TO USE GOTO
		if(m10digits==floating_trait::digits10)
		{
			unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
//rounding. 4 discard. 6 carry. 5 is complex
			if(ch==5)
			{
				if(ipart&1)
				{
					if(++ipart==floating_trait::carry10)
					{
						ipart=floating_trait::carry10/10;
						++m10e;
					}
					for(;;)
					{
						unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
						if(ch==21||ch==53)
						{
							phase2 = 0;
							break;
						}
						else if(9<ch)
						{
							phase2 = 2;
							break;
						}
					}
				}
				else
				{
					for(;;)
					{
						unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
						if(ch==21||ch==53)
						{
							phase2 = 0;
							break;
						}
						else if(ch)
						{
							if(9<ch)
							{
								phase2 = 2;
								break;
							}
							if(++ipart==floating_trait::carry10)
							{
								ipart=floating_trait::carry10/10;
								++m10e;
							}
							for(;;)
							{
								unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
								if(ch==static_cast<unsigned_char_type>(-2))
								{
									phase2 = 1;
									break;
								}
								else if(ch==21||ch==53)
								{
									phase2 = 0;
									break;
								}
								else if(9<ch)
									break;
							}
							break;
						}
					}
				}
			}
			else if(ch<10)
			{
				if(5<ch)
				{
					if(++ipart==floating_trait::carry10)
					{
						ipart=floating_trait::carry10/10;
						++m10e;
					}
				}
				for(;;)
				{
					unsigned_char_type const ch(static_cast<unsigned_char_type>(get<true>(in).first)-48);
					if(ch==21||ch==53)
					{
						phase2 = 0;
						break;
					}
					else if(9<ch)
					{
						phase2 = 2;
						break;
					}
				}
			}
			else if(ch==21||ch==53)
				phase2 = 0;
			else
				phase2 = 2;
		}
	}
	if(!phase2)
	{
		signed_exponent_type user_exp{};
		scan(in,user_exp);
		m10e+=user_exp;
		if(m10e<-308||308<m10e)
			throw std::overflow_error("exponent too large");
	}
	bool trailing_zeros{};
	using std::log2p1;
	signed_exponent_type e2(static_cast<signed_exponent_type>(log2p1(ipart))+m10e-(2+floating_trait::mantissa_bits));
	mantissa_type m2{};
	if(m10e<0)
	{
		auto const p5bm10e(pow5bits(-m10e));
		e2-=p5bm10e;
		m2=mul_shift(ipart,pow5<F,true>::inv_split[-m10e],e2-m10e+p5bm10e-1+floating_trait::pow5_bitcount);
		trailing_zeros=multiple_of_power_of_5(ipart,-m10e);
	}
	else
	{
		e2+=log2pow5(m10e);
		m2=mul_shift(ipart,pow5<F,true>::split[m10e],e2-m10e-pow5bits(m10e)+floating_trait::pow5_bitcount);
		trailing_zeros = e2 < m10e || multiple_of_power_of_2(ipart, e2 - m10e);
	}
	exponent_type ieee_e2(e2 + (floating_trait::bias-1) + log2p1(m2));
	if(ieee_e2<0)
		ieee_e2=0;
	signed_exponent_type shift((!ieee_e2?1:ieee_e2)-e2-(floating_trait::bias+floating_trait::mantissa_bits));
	trailing_zeros &= !(m2 & ((1L << (shift - 1)) - 1));
	bool last_removed_bit((m2>>(shift-1))&1);
	bool round_up((last_removed_bit) && (!trailing_zeros || ((m2 >> shift) & 1)));
	return bit_cast<F>(((((static_cast<mantissa_type>(negative)) << floating_trait::exponent_bits) | static_cast<mantissa_type>(ieee_e2)) << 
	floating_trait::mantissa_bits)|(((m2 >> shift) + round_up) & ((static_cast<mantissa_type>(1) << floating_trait::mantissa_bits) - 1)));
}

}