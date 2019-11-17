#pragma once

namespace fast_io
{

enum class seekdir
{
beg = 0,				//SEEK_SET
cur = 1,				//SEEK_CUR
end = 2,				//SEEK_END
};

template<typename T>
struct seek_type_t
{
	inline constexpr explicit seek_type_t() = default;
};

template<typename T>
inline constexpr seek_type_t<T> seek_type{};

}