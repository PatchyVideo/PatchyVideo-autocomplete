#pragma once

namespace fast_io
{

namespace open
{

struct mode
{
	std::size_t value;
	constexpr operator std::size_t() const
	{
		return value;
	}
	constexpr mode& operator|=(mode const& b)
	{
		value|=b.value;
		return *this;
	}
	constexpr mode(std::size_t val=0):value(val){}
};

inline static mode constexpr app{1};
inline static mode constexpr ate{1<<1};
inline static mode constexpr binary{1<<2};
inline static mode constexpr direct{1<<3};
inline static mode constexpr excl{1<<4};//C++ iostream currently still does not support "x"
inline static mode constexpr in{1<<5};
inline static mode constexpr out{1<<6};
inline static mode constexpr sync{1<<7};
inline static mode constexpr trunc{1<<8};

inline constexpr mode operator|(mode const& a,mode const& b)
{
	auto temp(a);
	temp|=b;
	return temp;
}

inline constexpr mode remove_ate(mode const& m)
{
	return {m.value&~ate.value};
}
inline constexpr mode remove_binary(mode const& m)
{
	return {m.value&~binary.value};
}

inline constexpr bool with_ate(mode const& m)
{
	return m.value&ate.value;
}
inline constexpr bool with_binary(mode const& m)
{
	return m.value&binary.value;
}

inline constexpr mode remove_direct(mode const& m)
{
	return {m.value&~direct.value};
}

inline constexpr bool with_direct(mode const& m)
{
	return m.value&direct.value;
}

inline constexpr mode remove_sync(mode const& m)
{
	return {m.value&~sync.value};
}

inline constexpr bool with_sync(mode const& m)
{
	return m.value&sync.value;
}

inline constexpr mode remove_ate_direct_sync(mode const& m)
{
	return {m.value&~ate.value&~direct.value&~sync.value};
}
inline constexpr mode remove_ate_binary_direct_sync(mode const& m)
{
	return {m.value&~ate.value&~binary.value&~direct.value&~sync.value};
}


inline auto constexpr c_style(mode const& m)
{
	using namespace std::string_view_literals;
	switch(remove_ate_direct_sync(m))
	{
//Action if file already exists;	Action if file does not exist;	c-style mode;	Explanation
//Read from start;	Failure to open;	"r";	Open a file for reading
	case in:
		return "r"sv;
//Destroy contents;	Create new;	"w";	Create a file for writing
	case out:
	case out|trunc:
		return "w"sv;
//Append to file;	Create new;	"a";	Append to a file
	case app:
	case out|app:
		return "a"sv;
//Read from start;	Error;	"r+";		Open a file for read/write
	case out|in:
		return "r+"sv;
//Destroy contents;	Create new;	"w+";	Create a file for read/write
	case out|in|trunc:
		return "w+"sv;
//Write to end;	Create new;	"a+";	Open a file for read/write
	case out|in|app:
	case in|app:
		return "a+"sv;
//Destroy contents;	Error;	"wx";	Create a file for writing
	case out|excl:
	case out|trunc|excl:
		return "wx"sv;
//Append to file;	Error;	"ax";	Append to a file
	case app|excl:
	case out|app|excl:
		return "ax"sv;
//Destroy contents;	Error;	"w+x";	Create a file for read/write
	case out|in|trunc|excl:
		return "w+x"sv;
//Write to end;	Error;	"a+x";	Open a file for read/write
	case out|in|app|excl:
	case in|app|excl:
		return "a+x"sv;
	break;
	
//binary support

//Action if file already exists;	Action if file does not exist;	c-style mode;	Explanation
//Read from start;	Failure to open;	"rb";	Open a file for reading
	case in|binary:
		return "rb"sv;
//Destroy contents;	Create new;	"wb";	Create a file for writing
	case out|binary:
	case out|trunc|binary:
		return "wb"sv;
//Append to file;	Create new;	"ab";	Append to a file
	case app|binary:
	case out|app|binary:
		return "ab"sv;
//Read from start;	Error;	"r+b";		Open a file for read/write
	case out|in|binary:
		return "r+b"sv;
//Destroy contents;	Create new;	"w+b";	Create a file for read/write
	case out|in|trunc|binary:
		return "w+b"sv;
//Write to end;	Create new;	"a+b";	Open a file for read/write
	case out|in|app|binary:
	case in|app|binary:
		return "a+b"sv;
//Destroy contents;	Error;	"wxb";	Create a file for writing
	case out|excl|binary:
	case out|trunc|excl|binary:
		return "wxb"sv;
//Append to file;	Error;	"axb";	Append to a file
	case app|excl|binary:
	case out|app|excl|binary:
		return "axb"sv;
//Destroy contents;	Error;	"w+xb";	Create a file for read/write
	case out|in|trunc|excl|binary:
		return "w+xb"sv;
//Write to end;	Error;	"a+xb";	Open a file for read/write
	case out|in|app|excl|binary:
	case in|app|excl|binary:
		return "a+xb"sv;
	break;
	default:
		static_assert(true, "unknown open mode");
	}
}

inline auto constexpr c_style(std::string_view csm)
{
	mode v{};
	bool extended(false);
	for(auto const& e : csm)
		if(e=='+')
			extended=true;
	for(auto const& e : csm)
		switch(e)
		{
			case 'a':
				v|=app;
				if(extended)
					v|=in|out;
			break;
			case 'b':
				v|=binary;
			break;
			case 'r':
				v|=in;
				if(extended)
					v|=out;
			break;
			case 'w':
				v|=out;
				if(extended)
					v|=in|trunc;
			break;
			case 'x':
				v|=excl;
			break;
			case '+':
			break;
			default:
				static_assert(true,"unknown C-style open mode");
		}
	return v;
}


template<std::size_t om>
struct interface_t
{
inline static fast_io::open::mode constexpr mode = {om};
explicit interface_t()=default;
};

template<std::size_t om>
inline interface_t<om> constexpr interface{};

}

struct native_interface_t
{
	explicit native_interface_t() = default;
};
inline native_interface_t constexpr native_interface;


}
