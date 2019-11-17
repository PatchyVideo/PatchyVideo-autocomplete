#pragma once

namespace fast_io
{
template<input_stream Ihandler,typename Buf>
class basic_ibuf;
template<output_stream Ohandler,typename Buf>
class basic_obuf;

template<typename T,std::size_t alignment=16384>
struct io_aligned_allocator
{
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	[[nodiscard]] inline T* allocate(std::size_t n)
	{
		return static_cast<T*>(operator new(n,std::align_val_t{alignment}));
	}
	inline void deallocate(T* p, std::size_t n) noexcept
	{
		operator delete(p,n,std::align_val_t{alignment});
	}
};

template<typename CharT,typename Allocator = io_aligned_allocator<CharT>,std::size_t buffer_size = 65536>
class basic_buf_handler
{
	Allocator alloc;
public:
	using char_type = CharT;
	using allocator_type = Allocator;
	char_type *beg,*curr,*end;
	explicit basic_buf_handler():beg(alloc.allocate(buffer_size)),end(beg+buffer_size){}
	basic_buf_handler& operator=(basic_buf_handler const&)=delete;
	basic_buf_handler(basic_buf_handler const&)=delete;
	static constexpr std::size_t size() {return buffer_size;}
	basic_buf_handler(basic_buf_handler&& m) noexcept:beg(m.beg),curr(m.curr),end(m.end)
	{
		m.beg=nullptr;
	}
	basic_buf_handler& operator=(basic_buf_handler&& m) noexcept
	{
		if(std::addressof(m)!=this)
		{
			alloc.deallocate(beg,buffer_size);
			beg=m.beg;
			curr=m.curr;
			end=m.end;
			m.beg=nullptr;
		}
		return *this;
	}
	~basic_buf_handler()
	{
		alloc.deallocate(beg,buffer_size);
	}
	Allocator get_allocator() const{	return alloc;}
	void swap(basic_buf_handler& o) noexcept
	{
		using std::swap;
		swap(alloc,o.alloc);
		swap(beg,o.beg);
		swap(curr,o.curr);
		swap(end,o.end);
	}
};

template<typename CharT,typename Allocator,std::size_t buffer_size>
inline void swap(basic_buf_handler<CharT,Allocator,buffer_size>& a,basic_buf_handler<CharT,Allocator,buffer_size>& b) noexcept
{
	a.swap(b);
}

namespace details::iobuf
{
template<typename T>
inline constexpr typename T::char_type* internal_mreads(T& ib,typename T::char_type *begin,typename T::char_type *end)
{
	using buffer_type = typename T::buffer_type;
	std::size_t n(end-begin);
	if(ib.ibuffer.end<n+ib.ibuffer.curr)			//cache miss
	{
		begin=std::uninitialized_copy(ib.ibuffer.curr,ib.ibuffer.end,begin);
		if(begin+buffer_type::size()<end)
		{
			begin=reads(ib.native_handle(),begin,end);
			if(begin!=end)
			{
				ib.ibuffer.end=ib.ibuffer.curr=ib.ibuffer.beg;
				return begin;
			}
		}
		ib.ibuffer.end=reads(ib.native_handle(),ib.ibuffer.beg,ib.ibuffer.beg+buffer_type::size());
		ib.ibuffer.curr=ib.ibuffer.beg;
		n=end-begin;
		std::size_t const sz(ib.ibuffer.end-ib.ibuffer.beg);
		if(sz<n)
			n=sz;
	}
	begin=std::uninitialized_copy_n(ib.ibuffer.curr,n,begin);
	ib.ibuffer.curr+=n;
	return begin;
}
}

template<input_stream Ihandler,typename Buf=basic_buf_handler<typename Ihandler::char_type>>
class basic_ibuf
{
public:
	Ihandler ih;
	Buf ibuffer;

	using native_handle_type = Ihandler;
	using buffer_type = Buf;
	using char_type = typename Buf::char_type;	
	template<typename... Args>
	requires std::constructible_from<Ihandler,Args...>
	basic_ibuf(Args&&... args):ih(std::forward<Args>(args)...){ibuffer.curr=ibuffer.end;}
	inline constexpr auto& native_handle()
	{
		return ih;
	}
};

template<input_stream Ihandler,typename Buf>
[[nodiscard]] inline constexpr auto ireserve(basic_ibuf<Ihandler,Buf>& ib,std::size_t size)->decltype(ib.ibuffer.curr)
{
	if(ib.ibuffer.end<=ib.ibuffer.curr+size)
		return nullptr;
	return ib.ibuffer.curr+=size;
}

template<input_stream Ihandler,typename Buf>
inline constexpr void irelease(basic_ibuf<Ihandler,Buf>& ib,std::size_t size)
{
	ib.ibuffer.curr-=size;
}

template<output_stream output,input_stream Ihandler,typename Buf>
inline constexpr void idump(output& out,basic_ibuf<Ihandler,Buf>& ib)
{
	writes(out,ib.ibuffer.curr,ib.ibuffer.end);
	ib.ibuffer.curr=ib.ibuffer.end;
}

template<input_stream Ihandler,typename Buf,std::contiguous_iterator Iter>
inline constexpr Iter reads(basic_ibuf<Ihandler,Buf>& ib,Iter begin,Iter end)
{
	using char_type = typename basic_ibuf<Ihandler,Buf>::char_type;
	auto b(static_cast<char_type*>(static_cast<void*>(std::to_address(begin))));
	return begin+(details::iobuf::internal_mreads(ib,b,static_cast<char_type*>(static_cast<void*>(std::to_address(end))))-b)/sizeof(*begin);
}

template<input_stream Ihandler,typename Buf>
inline constexpr auto get(basic_ibuf<Ihandler,Buf>& ib)
{
	using buffer_type = typename basic_ibuf<Ihandler,Buf>::buffer_type;
	if(ib.ibuffer.end==ib.ibuffer.curr)		//cache miss
	{
		if((ib.ibuffer.end=reads(ib.native_handle(),ib.ibuffer.beg,ib.ibuffer.beg+buffer_type::size()))==ib.ibuffer.beg)
		{
			ib.ibuffer.curr=ib.ibuffer.beg;
			throw eof();
		}
		ib.ibuffer.curr=ib.ibuffer.beg;
	}
	return *ib.ibuffer.curr++;
}

template<input_stream Ihandler,typename Buf>
inline constexpr std::pair<typename basic_ibuf<Ihandler,Buf>::char_type,bool> try_get(basic_ibuf<Ihandler,Buf>& ib)
{
	using buffer_type = typename basic_ibuf<Ihandler,Buf>::buffer_type;
	if(ib.ibuffer.end==ib.ibuffer.curr)		//cache miss
	{
		if((ib.ibuffer.end=reads(ib.native_handle(),ib.ibuffer.beg,ib.ibuffer.beg+buffer_type::size()))==ib.ibuffer.beg)
		{
			ib.ibuffer.curr=ib.ibuffer.beg;
			return {0,true};
		}
		ib.ibuffer.curr=ib.ibuffer.beg;
	}
	return {*ib.ibuffer.curr++,false};
}

template<input_stream Ihandler,typename Buf,typename... Args>
requires random_access_stream<Ihandler>
inline constexpr auto seek(basic_ibuf<Ihandler,Buf>& ib,Args&& ...args)
{
	ib.ibuffer.curr=ib.ibuffer.end;
	return seek(ib.native_handle(),std::forward<Args>(args)...);
}

template<zero_copy_input_stream Ihandler,typename Buf>
inline constexpr auto zero_copy_in_handle(basic_ibuf<Ihandler,Buf>& ib)
{
	return zero_copy_in_handle(ib.native_handle());
}

template<output_stream Ohandler,typename Buf=basic_buf_handler<typename Ohandler::char_type>>
class basic_obuf
{
	Ohandler oh;
public:
	Buf obuffer;
	inline constexpr void close_impl() noexcept
	try
	{
		if(obuffer.beg)
			writes(oh,obuffer.beg,obuffer.curr);
	}
	catch(...){}
public:
	using native_handle_type = Ohandler;
	using buffer_type = Buf;
	using char_type = typename Buf::char_type;
	template<typename... Args>
	requires std::constructible_from<Ohandler,Args...>
	basic_obuf(Args&&... args):oh(std::forward<Args>(args)...){obuffer.curr=obuffer.beg;}
	~basic_obuf()
	{
		close_impl();
	}
	basic_obuf& operator=(basic_obuf const&)=delete;
	basic_obuf(basic_obuf const&)=delete;
	basic_obuf(basic_obuf&& bmv) noexcept:oh(std::move(bmv.oh)),obuffer(std::move(bmv.obuffer)){}
	basic_obuf& operator=(basic_obuf&& b) noexcept
	{
		if(std::addressof(b)!=this)
		{
			close_impl();
			oh=std::move(b.oh);
			obuffer=std::move(b.obuffer);
		}
		return *this;
	}
	inline constexpr auto& native_handle()
	{
		return oh;
	}
};

template<output_stream Ohandler,typename Buf,std::contiguous_iterator Iter>
inline constexpr void writes(basic_obuf<Ohandler,Buf>& ob,Iter cbegini,Iter cendi)
{
	using char_type = typename basic_obuf<Ohandler,Buf>::char_type;
	auto cbegin(static_cast<char_type const*>(static_cast<void const*>(std::to_address(cbegini)))),cend(static_cast<char_type const*>(static_cast<void const*>(std::to_address(cendi))));
	std::size_t const n(ob.obuffer.end-ob.obuffer.curr);
	if(n<static_cast<std::size_t>(cend-cbegin))
	{
		std::uninitialized_copy_n(cbegin,n,ob.obuffer.curr);
		cbegin+=n;
		writes(ob.native_handle(),ob.obuffer.beg,ob.obuffer.end);
		if(cbegin+Buf::size()<cend)
		{
			writes(ob.native_handle(),cbegin,cend);
			ob.obuffer.curr=ob.obuffer.beg;
		}
		else
			ob.obuffer.curr=std::uninitialized_copy(cbegin,cend,ob.obuffer.beg);
	}
	else
		ob.obuffer.curr=std::uninitialized_copy(cbegin,cend,ob.obuffer.curr);
}

template<output_stream Ohandler,typename Buf>
inline constexpr void fill_nc(basic_obuf<Ohandler,Buf>& ob,std::size_t count,typename basic_obuf<Ohandler,Buf>::char_type const& ch)
{
	std::size_t const remain_space(static_cast<std::size_t>(ob.obuffer.end-ob.obuffer.curr));
	if(remain_space<=count)
	{
		std::fill(ob.obuffer.curr,ob.obuffer.end,ch);
		writes(ob.native_handle(),ob.obuffer.beg,ob.obuffer.end);
		count-=remain_space;
		constexpr auto buffer_size(Buf::size());
		std::size_t const times(count/buffer_size),remain(count%buffer_size);
		if(times)
		{
			std::fill(ob.obuffer.beg,ob.obuffer.curr,ch);
			for(std::size_t i(0);i!=times;++i)
				writes(ob.native_handle(),ob.obuffer.beg,ob.obuffer.end);
		}
		else
		{
			std::size_t cb(static_cast<std::size_t>(ob.obuffer.curr-ob.obuffer.beg));
			if(remain<cb)
				cb=remain;
			std::fill_n(ob.obuffer.beg,cb,ch);
		}
		ob.obuffer.curr=ob.obuffer.beg+remain;
		return;
	}
	ob.obuffer.curr=std::fill_n(ob.obuffer.curr,count,ch);
}

template<output_stream Ohandler,typename Buf>
inline constexpr void put(basic_obuf<Ohandler,Buf>& ob,typename basic_obuf<Ohandler,Buf>::char_type ch)
{
	if(ob.obuffer.curr==ob.obuffer.end)		//buffer full
	{
		writes(ob.native_handle(),ob.obuffer.beg,ob.obuffer.end);
		ob.obuffer.curr=ob.obuffer.beg;
	}
	*ob.obuffer.curr++=ch;
}
template<output_stream Ohandler,typename Buf>
inline constexpr void flush(basic_obuf<Ohandler,Buf>& ob)
{
	writes(ob.native_handle(),ob.obuffer.beg,ob.obuffer.curr);
	ob.obuffer.curr=ob.obuffer.beg;
//	flush(oh.native_handle());
}


template<output_stream Ohandler,typename Buf,typename... Args>
requires random_access_stream<Ohandler>
inline constexpr auto seek(basic_obuf<Ohandler,Buf>& ob,Args&& ...args)
{
	writes(ob.native_handle(),ob.obuffer.beg,ob.obuffer.curr);
	ob.obuffer.curr=ob.obuffer.beg;
	return seek(ob.native_handle(),std::forward<Args>(args)...);
}

template<output_stream Ohandler,typename Buf>
[[nodiscard]] inline constexpr auto oreserve(basic_obuf<Ohandler,Buf>& ob,std::size_t size) -> decltype(ob.obuffer.curr)
{
	if(ob.obuffer.end<=ob.obuffer.curr+size)
		return nullptr;
	return ob.obuffer.curr+=size;
}

template<output_stream Ohandler,typename Buf>
inline constexpr void orelease(basic_obuf<Ohandler,Buf>& ob,std::size_t size)
{
	ob.obuffer.curr-=size;
}

template<zero_copy_output_stream Ohandler,typename Buf>
inline constexpr void zero_copy_out_handle(basic_obuf<Ohandler,Buf>& ob)
{
	return zero_copy_out_handle(ob.native_handle());
}

namespace details
{
template<io_stream io_handler,typename Buf>
struct fake_basic_ihandler:basic_obuf<io_handler,Buf>
{
template<typename... Args>
requires std::constructible_from<io_handler,Args...>
fake_basic_ihandler(Args&& ...args):basic_obuf<io_handler,Buf>(std::forward<Args>(args)...){}
};

template<io_stream io_handler,typename Buf,std::contiguous_iterator Iter>
inline constexpr Iter reads(fake_basic_ihandler<io_handler,Buf>& iob,Iter begin,Iter end)
{
	return reads(iob.native_handle(),begin,end);
}

}

template<io_stream io_handler,typename Buf=basic_buf_handler<typename io_handler::char_type>>
class basic_iobuf
{
public:
	using native_handle_type = io_handler;
	using buffer_type = Buf;
	using char_type = typename Buf::char_type;
	basic_ibuf<details::fake_basic_ihandler<native_handle_type,Buf>> ibf;
	template<typename... Args>
	requires std::constructible_from<io_handler,Args...>
	basic_iobuf(Args&& ...args):ibf(std::forward<Args>(args)...){}
	inline constexpr auto& native_handle()
	{
		return ibf.native_handle().native_handle();
	}
	inline constexpr auto& ibuffer()
	{
		return ibf.ibuffer;
	}
	inline constexpr auto& obuffer()
	{
		return ibf.native_handle().obuffer;
	}
	friend inline constexpr void flush(basic_iobuf& iob)
	{
		flush(iob.ibf.native_handle());
	}
	friend inline constexpr void put(basic_iobuf& iob,char_type ch)
	{
		put(iob.ibf.native_handle(),ch);
	}
	template<std::contiguous_iterator Iter>
	friend inline constexpr void writes(basic_iobuf& iob,Iter begin,Iter end)
	{
		writes(iob.ibf.native_handle(),begin,end);
	}
	template<std::contiguous_iterator Iter>
	friend inline constexpr Iter reads(basic_iobuf& iob,Iter begin,Iter end)
	{
		return reads(iob.ibf,begin,end);
	}
	friend inline constexpr auto get(basic_iobuf& iob)
	{
		return get(iob.ibf);
	}
	friend inline constexpr auto& ibuffer(basic_iobuf& iob)
	{
		return ibuffer(iob.ibf);
	}
	friend inline constexpr void irelease(basic_iobuf& iob,std::size_t size)
	{
		irelease(iob.ibf,size);
	}
	friend inline constexpr auto ireserve(basic_iobuf& iob,std::size_t size)
	{
		return ireserve(iob.ibf,size);
	}
	template<output_stream output>
	friend inline constexpr auto& idump(output& out,basic_iobuf& iob)
	{
		return idump(out,iob.ibf);
	}
	friend inline constexpr auto& obuffer(basic_iobuf& iob)
	{
		return obuffer(iob.ibf.native_handle());
	}
	friend inline constexpr auto try_get(basic_iobuf& iob)
	{
		return try_get(iob.ibf);
	}
	friend inline constexpr void fill_nc(basic_iobuf& ob,std::size_t count,char_type const& ch)
	{
		fill_nc(ob.ibf.native_handle(),count,ch);
	}
	friend inline constexpr void orelease(basic_iobuf& ob,std::size_t size)
	{
		orelease(ob.ibf.native_handle(),size);
	}
	friend inline constexpr auto oreserve(basic_iobuf& iob,std::size_t size)
	{
		return oreserve(iob.ibf.native_handle(),size);
	}

};

template<zero_copy_input_stream Ihandler,typename Buf>
inline constexpr auto zero_copy_in_handle(basic_iobuf<Ihandler,Buf>& ib)
{
	return zero_copy_in_handle(ib.native_handle());
}

template<zero_copy_output_stream Ohandler,typename Buf>
inline constexpr auto zero_copy_out_handle(basic_iobuf<Ohandler,Buf>& ob)
{
	return zero_copy_out_handle(ob.native_handle());
}

template<io_stream io_handler,typename Buf,typename... Args>
requires random_access_stream<io_handler>
inline constexpr auto seek(basic_iobuf<io_handler,Buf>& iob,Args&& ...args)
{
	flush(iob.ibf.native_handle());
	return seek(iob.ibf,std::forward<Args>(args)...);
}

}
