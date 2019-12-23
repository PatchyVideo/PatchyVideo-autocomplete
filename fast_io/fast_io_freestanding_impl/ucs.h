#pragma once

namespace fast_io
{

namespace details
{

template<stream T,typename CharT>
struct ucs_char_size_max_cal
{
	explicit ucs_char_size_max_cal() = default;
	static constexpr bool value = sizeof(typename T::char_type)<sizeof(CharT);
};

}

template<stream T,typename CharT>
requires details::ucs_char_size_max_cal<T,CharT>::value
class ucs
{
	T ib;
public:
	using native_interface_t = T;
	using char_type = CharT;
	using native_char_type = typename native_interface_t::char_type;
	using unsigned_char_type = std::make_unsigned_t<char_type>;
	using unsigned_native_char_type = std::make_unsigned_t<native_char_type>;
private:
	inline constexpr char_type get_impl(unsigned_native_char_type ch)
	{
		auto constexpr ch_bits(sizeof(native_char_type)*8);
		union
		{
			unsigned_native_char_type ch;
			std::bitset<ch_bits> bts;
		}u{ch};
		if(!u.bts.test(ch_bits-1))
			return u.ch;
		auto constexpr ch_bits_m2(ch_bits-2);
		auto constexpr limitm1((static_cast<unsigned_native_char_type>(1)<<ch_bits_m2)-1);
		if(!u.bts.test(ch_bits_m2))
			throw std::runtime_error("not a utf8 character");
		u.bts.reset(ch_bits-1);
		std::size_t pos(ch_bits_m2-1);
		for(;pos<ch_bits&&u.bts.test(pos);--pos)
			u.bts.reset(pos);
		std::size_t bytes(ch_bits_m2-pos);
		unsigned_char_type converted_ch(u.ch);
		for(std::size_t i(0);i!=bytes;++i)
		{
			unsigned_native_char_type t(get(ib));
			if((t>>ch_bits_m2)==2)
				converted_ch=((converted_ch<<ch_bits_m2)|(t&limitm1))&((1<<((i+2)*6)) -1);
			else
				throw std::runtime_error("not a utf8 character");
		}
		return converted_ch;
	}
public:
	template<typename ...Args>
	requires std::constructible_from<T,Args...>
	constexpr ucs(Args&& ...args):ib(std::forward<Args>(args)...){}
	constexpr auto& native_handle()
	{
		return ib;
	}
	constexpr char_type mmget() requires character_input_stream<T>
	{
		return get_impl(get(ib));
	}
	constexpr std::pair<char_type,bool> mmtry_get() requires character_input_stream<T>
	{
		auto ch(get<true>(ib));
		if(ch.second)
			return {0,true};
		return {get_impl(ch.first),false};
	}
	constexpr void mmput(char_type ch) requires character_output_stream<T>
	{
		unsigned_native_char_type constexpr native_char_bits(8*sizeof(unsigned_native_char_type));
		unsigned_native_char_type constexpr fair(1<<(native_char_bits-1));
		unsigned_native_char_type constexpr utf8_limit(1<<(native_char_bits-2));
		if(ch<fair)
		{
			put(ib,static_cast<native_char_type>(ch));
			return;
		}
		std::array<unsigned_native_char_type,sizeof(char_type)/sizeof(unsigned_native_char_type)+1> v{};
		auto ed(v.data()+v.size());
		do
		{
			*--ed = (ch%utf8_limit)|fair;
		}
		while(ch/=utf8_limit);
		std::size_t v_elements(v.data()+v.size()-ed);
		if((1<<(native_char_bits-1-v_elements))<=(*ed&~fair))
		{
			--ed;
			++v_elements;
		}
		unsigned_native_char_type constexpr max_native_char_type(-1);
		*ed |= max_native_char_type>>(native_char_bits-v_elements-1)<<(native_char_bits-v_elements);
		send(ib,ed,v.data()+v.size());
	}
};


template<output_stream T,typename CharT>
inline constexpr void flush(ucs<T,CharT>& uc)
{
	flush(uc.native_handle());
}
template<character_output_stream T,typename char_type>
inline constexpr void put(ucs<T,char_type>& uc,typename ucs<T,char_type>::char_type ch)
{
	uc.mmput(ch);
}

template<character_output_stream T,typename char_type,std::forward_iterator Iter>
inline constexpr void send(ucs<T,char_type>& uc,Iter b,Iter e)
{
	define_send_by_put(uc,b,e);
}

template<bool err=false,character_input_stream T,typename char_type>
inline constexpr auto get(ucs<T,char_type>& uc)
{
	if constexpr(err)
		return uc.mmtry_get();
	else
		return uc.mmget();
}


template<character_input_stream T,typename char_type,std::forward_iterator Iter>
inline constexpr Iter receive(ucs<T,char_type>& uc,Iter b,Iter e)
{
	return define_receive_by_get(uc,b,e);
}

template<typename T>
inline constexpr void in_place_utf8_to_ucs(T& t,std::string_view view)
{
	basic_istring_view<std::string_view> ibsv(view);
	ucs<decltype(ibsv),typename T::value_type> uv(ibsv);
	getwhole(uv,t);
}

template<typename T=std::wstring>
inline constexpr auto utf8_to_ucs(std::string_view view)
{
	T t;
	in_place_utf8_to_ucs(t,view);
	return t;
}

namespace details
{
template<typename T>
inline void in_place_ucs_to_utf8(std::string& v,std::basic_string_view<T> view)
{
	v.clear();
	ucs<basic_ostring<std::string>,T> uv(std::move(v));
	send(uv,view.cbegin(),view.cend());
	v=std::move(uv.native_handle().str());
}

template<typename T>
inline std::string ucs_to_utf8(std::basic_string_view<T> view)
{
	ucs<basic_ostring<std::string>,T> uv;
	send(uv,view.cbegin(),view.cend());
	return std::move(uv.native_handle().str());
}
}

inline void in_place_ucs_to_utf8(std::string& v,std::wstring_view view)
{
	details::in_place_ucs_to_utf8(v,view);
}

inline std::string ucs_to_utf8(std::wstring_view v)
{
	return details::ucs_to_utf8(v);
}

inline void in_place_ucs_to_utf8(std::string& v,std::u16string_view view)
{
	details::in_place_ucs_to_utf8(v,view);
}

inline std::string ucs_to_utf8(std::u16string_view v)
{
	return details::ucs_to_utf8(v);
}

inline void in_place_ucs_to_utf8(std::string& v,std::u32string_view view)
{
	details::in_place_ucs_to_utf8(v,view);
}

inline std::string ucs_to_utf8(std::u32string_view v)
{
	return details::ucs_to_utf8(v);
}

}