#pragma once

#include<exception>

namespace fast_io
{

class eof:public std::exception
{
public:
	explicit eof()=default;
	char const* what() const noexcept{return "EOF";}
};


}
