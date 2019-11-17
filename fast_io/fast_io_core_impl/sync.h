#pragma once

namespace fast_io
{
template<output_stream output,typename ostr>			// the output device itself must be atomic
class basic_sync
{
public:
	using native_handle_type = output;
	using char_type = typename native_handle_type::char_type;
	using buffer_type = ostr;
private:
	native_handle_type handle;
	buffer_type mostr;
public:
	auto& buffer(){return mostr;}
	auto& native_handle() {return handle;}
	template<typename ...Args>
	requires std::constructible_from<output,Args...>
	basic_sync(Args&& ...args):handle(std::forward<Args>(args)...){}
};

template<output_stream output,typename ostr>
inline constexpr void flush(basic_sync<output,ostr>& sync)
{
	writes(sync.native_handle(),sync.buffer().str().cbegin(),sync.buffer().str().cend());
	sync.buffer().clear();
}

template<output_stream output,typename ostr>
inline constexpr auto oreserve(basic_sync<output,ostr>& sync)
{
	return oreserve(sync.buffer());
}
template<output_stream output,typename ostr>
inline constexpr void orelease(basic_sync<output,ostr>& sync)
{
	orelease(sync.buffer());
}


template<output_stream output,typename ostr>
inline constexpr auto osize(basic_sync<output,ostr>& sync)
{
	return osize(sync.buffer());
}

template<output_stream output,typename ostr,std::contiguous_iterator Iter>
inline constexpr void writes(basic_sync<output,ostr>& sync,Iter cbegin,Iter cend)
{
	writes(sync.buffer(),cbegin,cend);
}
template<output_stream output,typename ostr>
inline constexpr void put(basic_sync<output,ostr>& sync,typename output::char_type ch)
{
	put(sync.buffer(),ch);
}

template<output_stream output,typename ostr,typename... Args>
requires random_access_stream<output>
inline constexpr auto seek(basic_sync<output,ostr>& sync,Args&& ...args)
{
	flush(sync);
	return seek(sync.native_handle(),std::forward<Args>(args)...);
}

template<io_stream input,typename ostr,std::contiguous_iterator Iter>
inline constexpr Iter reads(basic_sync<input,ostr>& sync,Iter begin,Iter end)
{
	flush(sync);
	return reads(sync.native_handle(),begin,end);
}
template<io_stream input,typename ostr>
requires character_input_stream<input>
inline constexpr auto get(basic_sync<input,ostr>& sync)
{
	flush(sync);
	return get(sync.native_handle());
}

template<io_stream input,typename ostr>
requires character_input_stream<input>
inline constexpr auto try_get(basic_sync<input,ostr>& sync)
{
	flush(sync);
	return try_get(sync.native_handle());
}

template<output_stream output,typename ostr>
inline constexpr void fill_nc(basic_sync<output,ostr>& ob,std::size_t count,typename output::char_type const& ch)
{
	fill_nc(ob.buffer(),count,ch);
}

template<output_stream output,typename ostr>
class basic_fsync:public basic_sync<output,ostr>
{
	bool need_sync = true;
	void sync()
	try
	{
		if(need_sync)
			flush(static_cast<basic_sync<output,ostr>&>(*this));
	}
	catch(...){}
public:
	using native_handle_type = output;
	using char_type = typename native_handle_type::char_type;
	using buffer_type = ostr;
	template<typename ...Args>
	requires std::constructible_from<output,Args...>
	constexpr basic_fsync(Args&& ...args):basic_sync<output,ostr>(std::forward<Args>(args)...) {}
	basic_fsync(basic_fsync const&) = delete;
	basic_fsync& operator=(basic_fsync const&) = delete;
	basic_fsync(basic_fsync&& other) noexcept:basic_sync<output,ostr>(std::move(other)),need_sync(other.need_sync)
	{
		other.need_sync=false;
	}
	basic_fsync& operator=(basic_fsync&& other) noexcept
	{
		if(std::addressof(other)!=this)
		{
			sync();
			basic_sync<output,ostr>::operator=(std::move(other));
			other.need_sync=false;
		}
		return *this;
	}
	~basic_fsync(){sync();}
};

}