#pragma once

#include<exception>

namespace fast_io
{

class eof:public std::exception
{
public:
	explicit eof()=default;
	char const* what() const noexcept override
	{return reinterpret_cast<char const*>(u8"EOF");}
};


}
