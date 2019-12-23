#pragma once

namespace fast_io::crypto
{

template<character_output_stream T,typename strT = std::basic_string_view<typename T::char_type>>
class oone_time_pad
{
public:
	using char_type = typename T::char_type;
	using key_type = strT;
private:
	key_type strvw;
	T t;
	typename key_type::iterator iter;
	void write_remain()
	{
		if(iter!=strvw.cend())
			send(t,iter,strvw.cend());
	}
public:
    template<typename T1, typename ...Args>
	requires std::constructible_from<key_type, strT>&&std::constructible_from<T, Args...>
	inline constexpr oone_time_pad(T1&& t1,Args&& ...args):strvw(std::forward<T1>(t1)),t(std::forward<Args>(args)...),iter(strvw.cbegin()){}
	inline constexpr void mmput(char_type ch)
	{
//		since this is freestanding. precondition should not exist. include exception
//		if(iter==strvw.cend())
//			throw std::runtime_error("key is too short for one time pad");
		put(t,*iter^ch);
		++iter;
	}
	template<std::contiguous_iterator Iter>
	inline constexpr void mmsend(Iter b,Iter e)
	{
        auto pb(static_cast<char_type const*>(static_cast<void const*>(std::addressof(*b))));
		auto pi(pb), pe(pb+(e-b)*sizeof(*b)/sizeof(char_type));
		for(;pi!=pe;++pi)
			mmput(*pi);
	}
	inline constexpr void mmflush()
	{
		write_remain();
		iter=strvw.cend();
		flush(t);
	}
	inline constexpr auto& key()  const {return strvw;}
	inline constexpr auto& key() {return strvw;}
	constexpr oone_time_pad(oone_time_pad const&) = delete;
	constexpr oone_time_pad& operator=(oone_time_pad const&) = delete;
	constexpr oone_time_pad(oone_time_pad&& o) noexcept:strvw(std::move(strvw)),t(std::move(o.t)),iter(std::move(o.iter)){}
	constexpr oone_time_pad& operator=(oone_time_pad&& o) noexcept
	{
		if(std::addressof(o)!=this)
		{
			try
			{
				write_remain();
			}
			catch(...){}
			strvw=std::move(o.strvw);
			t=std::move(o.t);
			iter=std::move(o.iter);
		}
		return *this;
	}
	~oone_time_pad()
	try
	{
		write_remain();
	}
	catch(...)
	{
	}
};

template<character_output_stream T,typename strT>
inline constexpr void put(oone_time_pad<T,strT>& oon,typename oone_time_pad<T,strT>::char_type ch)
{
	oon.mmput(ch);
}
template<character_output_stream T,typename strT,std::contiguous_iterator Iter>
inline constexpr void send(oone_time_pad<T,strT>& oon,Iter cbegin,Iter cend)
{
	oon.mmsend(cbegin,cend);
}
template<character_output_stream T,typename strT>
inline constexpr void flush(oone_time_pad<T,strT>& oon)
{
	oon.mmflush(oon);
}

template<character_input_stream T,typename strv = std::basic_string_view<typename T::char_type>>
class ione_time_pad
{
	fast_io::basic_istring_view<strv> istr;
	T t;
public:
	using char_type = typename T::char_type;
	using key_type = strv;
	template<typename T1,typename ...Args>
	requires std::constructible_from<key_type, T1>&&std::constructible_from<T, Args...>
	ione_time_pad(T1&& t1,Args&& ...args):istr(std::forward<T1>(t1)),t(std::forward<Args>(args)...){}
	inline constexpr char_type mmget()
	{
		return get(t)^get(istr);
	}
	inline constexpr std::pair<char_type,bool> mmtry_get()
	{
		auto ch(try_get(t));
		if(ch.second)
			return {0,true};
		return {ch.first^get(istr),false};
	}
	template<std::contiguous_iterator Iter>
	inline constexpr Iter mmreceive(Iter b,Iter e)
	{
		auto pb(static_cast<char_type*>(static_cast<void*>(std::addressof(*b))));
		auto pe(pb+(e-b)*sizeof(*b)/sizeof(char_type));
		auto pi(pb);
		for(;pi!=pe;++pi)
		{
			auto ch(mmtry_get());
			if(ch.second)
				break;
			*pi=ch.first;
		}
		return b+(pi-pb)*sizeof(char_type)/sizeof(*b);
	}
	auto& key_istrview() {return istr;}
};

template<bool err=false,character_input_stream T,typename strT>
inline constexpr auto get(ione_time_pad<T,strT>& oon)
{
	if constexpr(err)
		return oon.mmtry_get();
	else
		return oon.mmget();
}

template<character_input_stream T,typename strT,std::contiguous_iterator Iter>
inline constexpr auto receive(ione_time_pad<T,strT>& oon,Iter begin,Iter end)
{
	return oon.mmreceive(begin,end);
}

}
