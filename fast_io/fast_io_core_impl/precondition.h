#pragma once

namespace fast_io
{

template<std::signed_integral offset_type,typename T,typename char_type, std::integral integr>
inline constexpr auto seek_precondition(integr i)
{
/*	std::make_unsigned_t<offset_type> constexpr offset_max(std::numeric_limits<offset_type>::max());
	i = i * static_cast<std::ptrdiff_t>(sizeof(T))/static_cast<std::ptrdiff_t>(sizeof(char_type));

	if constexpr(offset_max<static_cast<std::make_unsigned_t<integr>>(std::numeric_limits<integr>::max()))
	{
		if(offset_max<static_cast<std::make_unsigned_t<integr>>(i))
			throw std::runtime_error("index is not in range for seek type");
	}
	return static_cast<offset_type>(i);*/
	return static_cast<offset_type>(i * static_cast<std::ptrdiff_t>(sizeof(T))/static_cast<std::ptrdiff_t>(sizeof(char_type)));
}
}