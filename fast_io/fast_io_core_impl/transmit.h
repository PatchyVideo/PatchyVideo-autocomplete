#pragma once

namespace fast_io
{

template<output_stream output,input_stream input>
inline std::size_t bufferred_transmit(output& outp,input& inp)
{
	std::size_t transmitted_bytes(0);
	for(std::array<std::byte,65536> array;;)
	{
		auto p(receive(inp,array.data(),array.data()+array.size()));
		std::size_t transmitted_this_round(p-array.data());
		transmitted_bytes+=transmitted_this_round;
		send(outp,array.data(),p);
		if(!transmitted_this_round)
			return transmitted_bytes;
	}
}

template<output_stream output,input_stream input>
inline std::size_t bufferred_transmit(output& outp,input& inp,std::size_t bytes)
{
	std::size_t transmitted_bytes(0);
	for(std::array<std::byte,65536> array;bytes;)
	{
		std::size_t b(array.size());
		if(bytes<b)
			b=bytes;
		auto p(receive(inp,array.data(),array.data()+b));
		std::size_t read_bytes(p-array.data());
		send(outp,array.data(),p);
		transmitted_bytes+=read_bytes;
		if(read_bytes!=b)
			return transmitted_bytes;
		bytes-=read_bytes;
	}
	return transmitted_bytes;
}

template<output_stream output,input_stream input,typename... Args>
inline auto transmit(output& outp,input& inp,Args&& ...args)
{
	if constexpr((zero_copy_output_stream<output>||zero_copy_buffer_output_stream<output>)
		&&(zero_copy_buffer_input_stream<input>||zero_copy_input_stream<input>))
	{
		if constexpr(zero_copy_output_stream<output>&&zero_copy_input_stream<input>)
			return zero_copy_transmit(outp,inp,std::forward<Args>(args)...);
		else if constexpr(zero_copy_buffer_output_stream<output>&&zero_copy_input_stream<input>)
		{
			flush(outp);
			return zero_copy_transmit(outp.native_handle(),inp,std::forward<Args>(args)...);
		}
		else if constexpr(zero_copy_output_stream<output>&&zero_copy_buffer_input_stream<input>)
		{
			idump(outp,inp);
			return zero_copy_transmit(outp.native_handle(),std::forward<Args>(args)...);
		}
		else
		{
			idump(outp,inp);
			flush(outp);
			return zero_copy_transmit(outp.native_handle(),inp.native_handle(),std::forward<Args>(args)...);
		}
	}
	else
		return bufferred_transmit(outp,inp,std::forward<Args>(args)...);
}

}
