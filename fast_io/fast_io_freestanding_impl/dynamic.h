#pragma once

namespace fast_io
{

template<typename T>
class basic_dynamic_input_stream
{
public:
	using char_type = T;
private:
	struct base
	{
		virtual char_type* mmreads(char_type*,char_type*) = 0;
		virtual ~base() = default;
	};
	template<input_stream input>
	struct derv:base
	{
		input in;
		template<typename ...Args>
		derv(std::in_place_type_t<input>,Args&& ...args):in(std::forward<Args>(args)...){}
		char_type* mmreads(char_type* b,char_type* e) {return reads(in,b,e);}
	};
	std::unique_ptr<base> up;
public:
	template<input_stream P,typename ...Args>
	basic_dynamic_input_stream(std::in_place_type_t<P>,Args&& ...args):
		up(new derv<P>(std::in_place_type<P>,std::forward<Args>(args)...)){}
	template<std::contiguous_iterator Iter>
	friend inline Iter reads(basic_dynamic_input_stream& in,Iter b,Iter e)
	{
		char_type *pb(static_cast<char_type*>(static_cast<void*>(std::to_address(b))));
		char_type *pe(static_cast<char_type*>(static_cast<void*>(std::to_address(e))));
		return b+(in.up->mmreads(pb,pe)-pb)*sizeof(*b)/sizeof(char_type);
	}
};

template<typename T>
class basic_dynamic_output_stream
{
public:
	using char_type = T;
private:
	struct base
	{
		virtual void mmwrites(char_type const*,char_type const*) = 0;
		virtual void mmflush() = 0;
		virtual ~base() = default;
	};
	template<output_stream output>
	struct derv:base
	{
		output out;
		template<typename ...Args>
		derv(std::in_place_type_t<output>,Args&& ...args):out(std::forward<Args>(args)...){}
		void mmwrites(char_type const* b,char_type const* e) {writes(out,b,e);}
		void mmflush() {flush(out);}
	};
	std::unique_ptr<base> up;
public:
	template<output_stream P,typename ...Args>
	basic_dynamic_output_stream(std::in_place_type_t<P>,Args&& ...args):
		up(new derv<P>(std::in_place_type<P>,std::forward<Args>(args)...)){}
	template<std::contiguous_iterator Iter>
	inline friend void writes(basic_dynamic_output_stream& out,Iter b,Iter e)
	{
		out.up->mmwrites(static_cast<char_type const*>(static_cast<void const*>(std::to_address(b))),
							static_cast<char_type const*>(static_cast<void const*>(std::to_address(e))));
	}
	inline friend void flush(basic_dynamic_output_stream& out) { return out.up->mmflush();}
};


template<typename T>
class basic_dynamic_io_stream
{
public:
	using char_type = T;
private:
	struct base
	{
		virtual char_type* mmreads(char_type*,char_type*) = 0;
		virtual void mmwrites(char_type const*,char_type const*) = 0;
		virtual void mmflush() = 0;
		virtual ~base() = default;
	};
	template<io_stream in_output>
	struct derv:base
	{
		in_output io;
		template<typename ...Args>
		derv(std::in_place_type_t<in_output>,Args&& ...args):io(std::forward<Args>(args)...){}
		char_type* mmreads(char_type* b,char_type* e) {return reads(io,b,e);}
		void mmwrites(char_type const* b,char_type const* e) {writes(io,b,e);}
		void mmflush() {flush(io);}
	};
	std::unique_ptr<base> up;
public:
	template<io_stream P,typename ...Args>
	basic_dynamic_io_stream(std::in_place_type_t<P>,Args&& ...args):
		up(new derv<P>(std::in_place_type<P>,std::forward<Args>(args)...)){}
	template<std::contiguous_iterator Iter>
	friend inline Iter reads(basic_dynamic_io_stream& io,Iter b,Iter e)
	{
		char_type *pb(static_cast<char_type*>(static_cast<void*>(std::to_address(b))));
		char_type *pe(static_cast<char_type*>(static_cast<void*>(std::to_address(e))));
		return b+(io.up->mmreads(pb,pe)-pb)*sizeof(*b)/sizeof(char_type);
	}
	template<std::contiguous_iterator Iter>
	inline friend void writes(basic_dynamic_io_stream& io,Iter b,Iter e)
	{
		io.up->mmwrites(static_cast<char_type const*>(static_cast<void const*>(std::to_address(b))),
							static_cast<char_type const*>(static_cast<void const*>(std::to_address(e))));
	}
	inline friend void flush(basic_dynamic_io_stream& io) { return io.up->mmflush();}
};


using dynamic_input_stream = basic_dynamic_input_stream<char>;
using dynamic_output_stream = basic_dynamic_output_stream<char>;
using dynamic_io_stream = basic_dynamic_io_stream<char>;
}