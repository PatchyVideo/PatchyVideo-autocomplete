
#include "fast_io/fast_io.h"
#include "fast_io/fast_io_network.h"
#include "fast_io/fast_io_device.h"

#include "tree.h"

template<typename T>
void ignore(T&&)
{}

/*
*   POST /addtag         n tagid count cat ...  return ""
*   POST /addword        n tagid word ...       return ""
*   POST /setcount       n tagid count ...      return ""
*   POST /setcountdiff   n tagid diff ...       return ""
*   POST /deltag         tagid                  return ""
*   POST /delword        word                   return ""
*   GET  /?q=<prefix>&n=<max_words>             return JSON[{word,category,count},...]
*/

inline constexpr std::uint64_t hash(std::string_view str)
{
	std::uint64_t ret = 5381;

	for (auto const &ch : str)
		ret = ((ret << 5) + ret) + ch; /* hash * 33 + c */

	return ret;
}

auto response_header{"HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nContent-Length: "};
auto response_header_json{"HTTP/1.1 200 OK\nContent-Type: application/json; charset=utf-8\nContent-Length: "};

auto response404{"HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=utf-8\nContent-Length: 0\n\n"};
auto response405{"HTTP/1.1 405 Method Not Allowed\nContent-Type: text/html; charset=utf-8\nContent-Length: 0\n\n"};
auto response500{"HTTP/1.1 500 Internal Server Error\nContent-Type: text/html; charset=utf-8\nContent-Length: 0\n\n"};

enum class RequestMethod
{
	GET,
	POST
};

template<typename IsSpace, fast_io::character_input_stream input>
inline void read_split(input &in, std::basic_string<typename input::char_type> &str)
{
	IsSpace func;
	str.clear();
	for (decltype(try_get(in)) ch; !(ch = try_get(in)).second && !func(ch.first); str.push_back(ch.first));
}

inline uint8_t hex2int(char const &ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	throw std::runtime_error("not a hex");
}

inline auto decode_url(std::string const &url)
{
	std::string out;
	fast_io::istring_view url_isv(url);
	for (decltype(fast_io::try_get(url_isv)) ch;;)
	{
		ch = try_get(url_isv);
		if (ch.second)
			break;
		if (ch.first == '%')
		{
			uint8_t ch2;
			auto hex1(try_get(url_isv));
			auto hex2(try_get(url_isv));
			if (hex1.second || hex2.second)
				throw std::runtime_error("decode failed");
			ch2 = (hex2int(hex1.first) << 4) | hex2int(hex2.first);
			out.push_back(static_cast<char>(ch2));
		}
		else
			out.push_back(ch.first);
	}
	return out;
}

inline auto parse_path(std::string const &raw_path)
{
	std::unordered_map<std::string, std::string> params;
	fast_io::istring_view path_isv(raw_path);
	auto is_question([](auto const &ch) {return ch == '?'; });
	auto is_amp_or_equ([](auto const &ch) {return ch == '&' || ch == '='; });

	std::string path;
	read_split<decltype(is_question)>(path_isv, path);
	for (std::string key, val;;)
	{
		read_split<decltype(is_amp_or_equ)>(path_isv, key);
		read_split<decltype(is_amp_or_equ)>(path_isv, val);
		if (key.size() > 0)
			params.emplace(std::move(key), decode_url(std::move(val)));
		else
			break;
	}

	return std::make_pair(path, params);
}


class http_error : public std::runtime_error
{
public:
	int code;
public:
	http_error(int code) : std::runtime_error("http error"), code(code)
	{}
};

void abort(int code = 404)
{
	throw http_error(code);
}

template<fast_io::character_output_stream output>
inline void handle_request_q(output &out, std::unordered_map<std::string, std::string> const &params)
{
	std::string prefix(params.at("q"));
	std::string max_words_str("10");
	if (params.count("n"))
		max_words_str = params.at("n");
	if (prefix.size() < 1)
		abort(400);
	std::uint32_t max_words{10};
	fast_io::istring_view isv(max_words_str);
	scan(isv, max_words);
	if (max_words > 100 || max_words == 0)
		max_words = 10;

	//std::vector<Keywords *>
	auto query_result(QueryWord(prefix, max_words));
	print(out, "[");
	for (std::size_t i(0); i != query_result.size(); ++i)
	{
		auto const& key(*query_result[i]);
		print(out, "{\"tag\":\"", key.keyword, "\",\"cat\":", g_tags[key.tagid]->category, ",\"cnt\":", g_tags[key.tagid]->count, "}");
		if (i != query_result.size() - 1)
			print(out, ",");
	}
	print(out, "]");
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_addtag(output &out, input &content)
{
	ignore(out);
	std::uint32_t tagid;
	std::uint32_t count;
	std::uint32_t cat;
	std::size_t n(0);
	scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		scan(content, tagid, count, cat);
		AddTag(tagid, count, cat);
	}
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_addword(output &out, input &content)
{
	ignore(out);
	std::uint32_t tagid;
	std::string word;
	std::size_t n(0);
	scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		scan(content, tagid, word);
		if (word.size() < 2 || tagid >= g_tags.size())
			return;

		AddKeyword(tagid, word);
	}
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_setcount(output &out, input &content)
{
	ignore(out);
	std::uint32_t tagid;
	std::uint32_t count;
	std::size_t n(0);
	scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		scan(content, tagid, count);
		if (tagid >= g_tags.size())
			return;

		UpdateTagCount(tagid, count);
	}
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_setcountdiff(output &out, input &content)
{
	ignore(out);
	std::uint32_t tagid;
	std::int32_t diff;
	std::size_t n(0);
	scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		scan(content, tagid, diff);
		if (tagid >= g_tags.size())
			return;

		UpdateTagCountDiff(tagid, diff);
	}
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_deltag(output &out, input &content)
{
	ignore(out);
	std::uint32_t tagid;
	scan(content, tagid);
	if (tagid >= g_tags.size())
			return;

	DeleteTag(tagid);
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_delword(output &out, input &content)
{
	ignore(out);
	std::string word;
	scan(content, word);
	if (word.size() < 2)
		return;

	DeleteKeyword(word);
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request(output &out, input &content, RequestMethod method, std::string const &path, std::unordered_map<std::string, std::string> const &params)
{
	std::string response_body;
	fast_io::basic_ostring<std::string> response_body_stream(std::move(response_body));
	auto path_hashed(hash(path));
	switch (method)
	{
	case RequestMethod::GET:
		if (path != "/")
			abort(404);
		handle_request_q(response_body_stream, params);
		break;
	case RequestMethod::POST:
		switch (path_hashed)
		{
		case hash("/addtag"):
			handle_request_addtag(response_body_stream, content);
			break;
		case hash("/addword"):
			handle_request_addword(response_body_stream, content);
			break;
		case hash("/setcount"):
			handle_request_setcount(response_body_stream, content);
			break;
		case hash("/setcountdiff"):
			handle_request_setcountdiff(response_body_stream, content);
			break;
		case hash("/deltag"):
			handle_request_deltag(response_body_stream, content);
			break;
		case hash("/delword"):
			handle_request_delword(response_body_stream, content);
			break;
		default:
			abort(404);
		}
		break;
	}

	print(out, response_header_json);
	print(out, response_body_stream.str().size());
	print(out, "\n\n");
	print(out, response_body_stream.str());
}

int main()
try
{
	fast_io::server hd(5002, fast_io::sock::type::stream);
	for (;;)
		try
	{
		fast_io::acceptor_buf client_stream(hd);
		try
		{
			auto request_header(scan_http_header(client_stream));
			RequestMethod method;
			std::string path_version;
			if (request_header.count("POST"))
			{
				method = RequestMethod::POST;
				path_version = request_header["POST"];
			}
			else if (request_header.count("GET"))
			{
				method = RequestMethod::GET;
				path_version = request_header["GET"];
			}
			else
			{
				print(client_stream, response405);
				continue;
			}

			std::string raw_path;
			fast_io::istring_view isv(path_version);
			scan(isv, raw_path);

			auto const &[path, params] = parse_path(raw_path);

			handle_request(client_stream, client_stream, method, path, params);
		}
		catch (http_error const &ex)
		{
			print(client_stream, response404);
		}
		catch (std::exception const &e)
		{
			println(fast_io::err, e);
			print(client_stream, response500);
		}
	}
	catch (std::exception const &e)
	{
		println(fast_io::err, e);
	}
	return 0;
}
catch (std::exception const &e)
{
	println(fast_io::err, e);
	return 1;
}
