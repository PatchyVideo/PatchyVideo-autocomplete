#pragma once

namespace fast_io
{

template<std::integral T>
class basic_dynamic_base
{
public:
	using char_type = T;
private:
	struct base
	{
		virtual char_type* receive_impl(char_type*,char_type*) = 0;
		virtual void send_impl(char_type const*,char_type const*) = 0;
		virtual void flush_impl() = 0;
		virtual char_type* oreserve_impl(std::size_t) = 0;
		virtual void orelease_impl(std::size_t) = 0;
		virtual base* clone() = 0;
		virtual ~base() = default;
	};
	template<stream stm>
	struct derv:base
	{
		stm io;
		template<typename ...Args>
		derv(std::in_place_type_t<stm>,Args&& ...args):io(std::forward<Args>(args)...){}
		char_type* receive_impl(char_type* b,char_type* e) override
		{
			if constexpr(input_stream<stm>)
				return receive(io,b,e);
			else
				throw std::system_error(EPERM,std::generic_category());
		}
		void send_impl(char_type const* b,char_type const* e) override
		{
			if constexpr(output_stream<stm>)
				send(io,b,e);
			else
				throw std::system_error(EPERM,std::generic_category());
		}
		void flush_impl() override
		{
			if constexpr(output_stream<stm>)
				flush(io);
			else
				throw std::system_error(EPERM,std::generic_category());
		}
		char_type* oreserve_impl(std::size_t sz) override
		{
			if constexpr(buffer_output_stream<stm>)
			{
				if constexpr(std::contiguous_iterator<std::remove_cvref_t<decltype(oreserve(io,sz))>>)
					return std::to_address(oreserve(io,sz));
				else
					return nullptr;
			}
			else
				return nullptr;
		}
		void orelease_impl(std::size_t sz) override
		{
			if constexpr(buffer_output_stream<stm>)
				orelease(io,sz);
		}
		base* clone() override
		{
			if constexpr(std::copyable<stm>)
				return new derv<stm>(std::in_place_type<stm>,io);
			else
				throw std::system_error(EPERM,std::generic_category());
		}
	};
	base* ptr=nullptr;
public:
	using opaque_base = base;
	basic_dynamic_base()=default;
	basic_dynamic_base(void* p):ptr(bit_cast<base*>(p)){}
	auto release() noexcept
	{
		auto temp(ptr);
		ptr=nullptr;
		return temp;
	}
	auto opaque_base_pointer()
	{
		return ptr;
	}
	auto opaque_base_pointer() const
	{
		return ptr;
	}
	basic_dynamic_base(basic_dynamic_base const& b):basic_dynamic_base(b.ptr->clone()){}
	basic_dynamic_base& operator=(basic_dynamic_base const& b)
	{
		auto newp(b.ptr->clone());
		delete ptr;
		ptr=newp;
		return *this;
	}
	basic_dynamic_base(basic_dynamic_base&& b) noexcept:ptr(b.ptr)
	{
		b.ptr=nullptr;
	}
	basic_dynamic_base& operator=(basic_dynamic_base&& b) noexcept
	{
		if(b.ptr!=ptr)
		{
			delete ptr;
			ptr=b.ptr;
			b.ptr=nullptr;
		}
		return *this;
	}
	template<stream P>
	requires (!std::same_as<basic_dynamic_base,P>)
	basic_dynamic_base(P p):ptr(new derv<P>(std::in_place_type<P>,std::move(p))){}
	template<stream P,typename ...Args>
	basic_dynamic_base(std::in_place_type_t<P>,Args&& ...args):
		ptr(new derv<P>(std::in_place_type<P>,std::forward<Args>(args)...)){}
};

template<std::integral T>
struct basic_dynamic_stream:basic_dynamic_base<T>
{
public:
	using basic_dynamic_base<T>::char_type;
	template<typename ...Args>
	basic_dynamic_stream(Args&& ...args):basic_dynamic_base<T>(std::forward<Args>(args)...){}
	~basic_dynamic_stream()
	{
		delete this->opaque_base_pointer();
	}
};
template<std::integral char_type,std::contiguous_iterator Iter>
inline Iter receive(basic_dynamic_base<char_type>& io,Iter b,Iter e)
{
	char_type *pb(static_cast<char_type*>(static_cast<void*>(std::to_address(b))));
	char_type *pe(static_cast<char_type*>(static_cast<void*>(std::to_address(e))));
	return b+(io.opaque_base_pointer()->receive_impl(pb,pe)-pb)*sizeof(*b)/sizeof(char_type);
}

template<std::integral char_type,std::contiguous_iterator Iter>
inline void send(basic_dynamic_base<char_type>& io,Iter b,Iter e)
{
	io.opaque_base_pointer()->send_impl(static_cast<char_type const*>(static_cast<void const*>(std::to_address(b))),
			static_cast<char_type const*>(static_cast<void const*>(std::to_address(e))));
}

template<std::integral char_type>
inline auto oreserve(basic_dynamic_base<char_type>& io,std::size_t sz)
{
	return io.opaque_base_pointer()->oreserve_impl(sz);
}

template<std::integral char_type>
inline void orelease(basic_dynamic_base<char_type>& io,std::size_t sz)
{
	return io.opaque_base_pointer()->orelease_impl(sz);
}

template<std::integral char_type>
inline void flush(basic_dynamic_base<char_type>& io) { return io.opaque_base_pointer()->flush_impl();}

using dynamic_base = basic_dynamic_base<char>;

using dynamic_stream = basic_dynamic_stream<char>;


}