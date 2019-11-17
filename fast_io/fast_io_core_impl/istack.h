#pragma once

namespace fast_io
{

template<input_stream input,std::size_t N=16,typename Array=std::array<typename input::char_type,N>>
class istack
{
public:
	using native_handle_type = input;
	using array_type = Array;
	using char_type = typename array_type::value_type;
	array_type array;
	char_type *curr=nullptr,*end=nullptr;
	input ih;
	template<typename... Args>
	requires std::constructible_from<input,Args...>
	istack(Args&&... args):curr(array.data()),end(array.data()),ih(std::forward<Args>(args)...){}
	inline constexpr auto& native_handle()
	{
		return ih;
	}

};

namespace details::istack
{
template<typename T>
inline constexpr typename T::char_type* internal_mreads(T& ib,typename T::char_type *begin,typename T::char_type *end)
{
	std::size_t n(end-begin);
	if(static_cast<std::size_t>(ib.end-ib.curr)<n)			//cache miss
	{
		begin=std::uninitialized_copy(ib.curr,ib.end,begin);
		if(begin+ib.array.size()<end)
		{
			begin=reads(ib.native_handle(),begin,end);
			if(begin!=end)
			{
				ib.end=ib.curr=ib.array.data();
				return begin;
			}
		}
		ib.end=reads(ib.native_handle(),ib.beg,ib.beg+ib.size);
		ib.curr=ib.beg;
		n=end-begin;
		std::size_t const sz(ib.end-ib.array.data());
		if(sz<n)
			n=sz;
	}
	begin=std::uninitialized_copy_n(ib.curr,n,begin);
	ib.curr+=n;
	return begin;
}
}

template<input_stream Ihandler,std::size_t N>
[[nodiscard]] inline constexpr auto ireserve(istack<Ihandler,N>& ib,std::size_t size)->decltype(ib.curr)
{
	if(ib.end<=ib.curr+size)
		return nullptr;
	return ib.curr+=size;
}

template<input_stream Ihandler,std::size_t N>
inline constexpr void irelease(istack<Ihandler,N>& ib,std::size_t size)
{
	ib.curr-=size;
}


template<output_stream output,input_stream Ihandler,std::size_t N>
inline constexpr void idump(output& out,istack<Ihandler,N>& ib)
{
	writes(out,ib.curr,ib.end);
	ib.curr=ib.end;
}

template<input_stream Ihandler,std::size_t N,std::contiguous_iterator Iter>
inline constexpr Iter reads(istack<Ihandler,N>& ib,Iter begin,Iter end)
{
	using char_type = typename istack<Ihandler,N>::char_type;
	auto b(static_cast<char_type*>(static_cast<void*>(std::to_address(begin))));
	return begin+(details::istack::internal_mreads(ib,b,static_cast<char_type*>(static_cast<void*>(std::to_address(end))))-b)/sizeof(*begin);
}

template<input_stream Ihandler,std::size_t N>
inline constexpr auto get(istack<Ihandler,N>& ib)
{
	if(ib.end==ib.curr)		//cache miss
	{
		if((ib.end=reads(ib.native_handle(),ib.array.data(),ib.array.data()+ib.array.size()))==ib.array.data())
		{
			ib.curr=ib.array.data();
			throw eof();
		}
		ib.curr=ib.array.data();
	}
	return *ib.curr++;
}

template<input_stream Ihandler,std::size_t N>
inline constexpr std::pair<typename istack<Ihandler,N>::char_type,bool> try_get(istack<Ihandler,N>& ib)
{
	if(ib.end==ib.curr)		//cache miss
	{
		if((ib.end=reads(ib.native_handle(),ib.array.data(),ib.array.data()+ib.array.size()))==ib.array.data())
		{
			ib.curr=ib.array.data();
			return {0,true};
		}
		ib.curr=ib.array.data();
	}
	return {*ib.curr++,false};
}

template<input_stream Ihandler,std::size_t N,typename... Args>
requires random_access_stream<Ihandler>
inline constexpr auto seek(istack<Ihandler,N>& ib,Args&& ...args)
{
	ib.curr=ib.end;
	return seek(ib.native_handle(),std::forward<Args>(args)...);
}

template<input_stream Ihandler,std::size_t N>
requires output_stream<Ihandler>
inline constexpr void flush(istack<Ihandler,N>& ib)
{
	flush(ib.native_handle());
}

template<input_stream Ihandler,std::size_t N,std::contiguous_iterator Iter>
requires output_stream<Ihandler>
inline constexpr void writes(istack<Ihandler,N>& ib,Iter cbegin,Iter cend)
{
	writes(ib.native_handle(),cbegin,cend);
}

template<input_stream Ihandler,std::size_t N>
requires zero_copy_output_stream<Ihandler>
inline constexpr auto zero_copy_out_handle(istack<Ihandler,N>& ib)
{
	return zero_copy_out_handle(ib.native_handle());
}

}