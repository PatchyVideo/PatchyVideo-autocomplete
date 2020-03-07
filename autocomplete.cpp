
#include "fast_io/fast_io.h"
#include "fast_io/fast_io_async.h"
#include "fast_io/fast_io_device.h"
#include "fast_io/fast_io_network.h"

#include "tree.h"

template<typename T>
void ignore(T&&)
{}

/*
*   POST /addtag         n tagid count cat ...    return ""
*   POST /addword        n tagid word lang ...    return ""
*   POST /setcount       n tagid count ...        return ""
*   POST /setcountdiff   n tagid diff ...         return ""
*   POST /setcat         n tagid cat ...          return ""
*   POST /deltag         tagid                    return ""
*   POST /delword        word                     return ""
*   GET  /?q=<prefix>&n=<max_words>&l=<lang>      return JSON[{word,category,count},...]
*   GET  /ql?q=<prefix>&n=<max_words>             return JSON[{category,count,langs:[{language,word},...],alias:[word,...]},...]
*/

inline constexpr std::uint64_t hash(std::string_view str)
{
	std::uint64_t ret = 5381;

	for (auto const &ch : str)
		ret = ((ret << 5) + ret) + ch; /* hash * 33 + c */

	return ret;
}

auto response_header{u8"HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nContent-Length: "};
auto response_header_json{u8"HTTP/1.1 200 OK\nContent-Type: application/json; charset=utf-8\nContent-Length: "};

auto response404{u8"HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=utf-8\nContent-Length: 0\n\n"};
auto response405{u8"HTTP/1.1 405 Method Not Allowed\nContent-Type: text/html; charset=utf-8\nContent-Length: 0\n\n"};
auto response500{u8"HTTP/1.1 500 Internal Server Error\nContent-Type: text/html; charset=utf-8\nContent-Length: 0\n\n"};

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
	for (decltype(fast_io::get<true>(in)) ch; !(ch = fast_io::get<true>(in)).second && !func(ch.first); str.push_back(ch.first));
}

inline constexpr std::uint8_t hex2int(char const &ch)
{
	std::uint8_t ch_0(static_cast<std::uint8_t>(ch) - static_cast<std::uint8_t>('0'));
	std::uint8_t ch_a(static_cast<std::uint8_t>(ch) - static_cast<std::uint8_t>('a'));
	std::uint8_t ch_A(static_cast<std::uint8_t>(ch) - static_cast<std::uint8_t>('A'));
	if (ch_0 < 10)
		return ch_0;
	if (ch_a < 16)
		return ch_a + 10;
	if (ch_A < 16)
		return ch_A + 10;
	throw std::runtime_error("not a hex");
}

inline auto decode_url(std::string const &url)
{
	std::string out;
	fast_io::istring_view url_isv(url);
	for (decltype(fast_io::get<true>(url_isv)) ch;;)
	{
		ch = fast_io::get<true>(url_isv);
		if (ch.second)
			break;
		if (ch.first == '%')
		{
			uint8_t ch2;
			auto hex1(fast_io::get<true>(url_isv));
			auto hex2(fast_io::get<true>(url_isv));
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
	std::string user_language("0");
	if (params.contains("n"))
		max_words_str = params.at("n");
	if(params.contains("l"))
		user_language = params.at("l");
	if (prefix.size() < 1)
		abort(400);
	std::uint32_t max_words{10};
	std::uint32_t user_lang_index{0};
	fast_io::istring_view isv(max_words_str);
	scan(isv, max_words);
	fast_io::istring_view isv_lang(user_language);
	scan(isv_lang, user_lang_index);
	if (max_words > 100 || max_words == 0)
		max_words = 10;

	//std::vector<Keywords *>
	auto query_result(QueryWord(prefix, max_words, user_lang_index));
	print(out, u8"[");
	for (std::size_t i(0); i != query_result.size(); ++i)
	{
		auto const& key(*query_result[i]);
		print(out, u8"{\"tag\":\"", key.keyword, u8"\",\"cat\":", g_tags[key.tagid]->category, u8",\"cnt\":", g_tags[key.tagid]->count, u8"}");
		if (i != query_result.size() - 1)
			print(out, u8",");
	}
	print(out, u8"]");
}

template<fast_io::character_output_stream output>
inline void handle_request_ql(output &out, std::unordered_map<std::string, std::string> const &params)
{
	std::string prefix(params.at("q"));
	std::string max_words_str("10");
	if (params.contains("n"))
		max_words_str = params.at("n");
	if (prefix.size() < 1)
		abort(400);
	std::uint32_t max_words{10};
	std::uint32_t user_lang_index{0};
	fast_io::istring_view isv(max_words_str);
	scan(isv, max_words);
	if (max_words > 100 || max_words == 0)
		max_words = 10;

	//std::vector<Tag *>
	auto query_result(QueryWordTagObject(prefix, max_words));
	print(out, u8"[");
	for (std::size_t i(0); i != query_result.size(); ++i)
	{
		auto const& tagobj(*query_result[i]);
		print(out, u8"{");
		//print(out, u8"{\"tag\":\"", key.keyword, u8"\",\"cat\":", g_tags[key.tagid]->category, u8",\"cnt\":", g_tags[key.tagid]->count, u8"}");
		print(out, u8"\"cat\":", tagobj.category, u8",");
		print(out, u8"\"cnt\":", tagobj.count, u8",");
		print(out, u8"\"langs\":[");
		std::size_t cnt(0);
		for (auto const& [id, word] : tagobj.lang_keywords) {
			print(out, u8"{\"l\":", id, u8",\"w\":\"", word->keyword, u8"\"}");
			if (++cnt < tagobj.lang_keywords.size())
				print(out, u8",");
		}
		print(out, u8"],");
		cnt = 0;
		print(out, u8"\"alias\":[");
		for (auto const& word : tagobj.alias_keywords) {
			print(out, u8"\"", word->keyword, u8"\"");
			if (++cnt < tagobj.alias_keywords.size())
				print(out, u8",");
		}
		print(out, u8"]");
		print(out, u8"}");
		if (i != query_result.size() - 1)
			print(out, u8",");
	}
	print(out, u8"]");
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
	std::string lang;
	std::size_t n(0);
	scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		scan(content, tagid, word, lang);
		if (word.size() < 2 || tagid >= g_tags.size() || lang.size() != 3)
			return;

		AddKeyword(tagid, word, GetLanguageIndex(lang));
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
inline void handle_request_setcat(output &out, input &content)
{
	ignore(out);
	std::uint32_t tagid;
	std::uint32_t cat;
	std::size_t n(0);
	scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		scan(content, tagid, cat);
		if (tagid >= g_tags.size())
			return;

		UpdateTagCategory(tagid, cat);
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
		switch (path_hashed)
		{
		case hash("/"):
			handle_request_q(response_body_stream, params);
			break;
		case hash("/ql"):
			handle_request_ql(response_body_stream, params);
			break;
		}
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
		case hash("/setcat"):
			handle_request_setcat(response_body_stream, content);
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
	print(out, u8"\n\n");
	print(out, response_body_stream.str());
}

void handle_connection(fast_io::acceptor_buf& client_stream)
{
	try
	{
		auto request_header(scan_http_header(client_stream));
		RequestMethod method;
		std::string path_version;
		if (request_header.contains("POST"))
		{
			method = RequestMethod::POST;
			path_version = request_header["POST"];
		}
		else if (request_header.contains("GET"))
		{
			method = RequestMethod::GET;
			path_version = request_header["GET"];
		}
		else
		{
			print(client_stream, response405);
			return;
		}

		std::string raw_path;
		fast_io::istring_view isv(path_version);
		scan(isv, raw_path);

		auto const [path, params] = parse_path(raw_path);

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

int main()
try
{
	InitRootTrieNodes();
	fast_io::server hd(5002, fast_io::sock::type::stream);
	for (;;) try
	{
		fast_io::acceptor_buf client_stream(hd);
		handle_connection(client_stream);
	}
	catch (std::exception const &e)
	{
		println(fast_io::err, e);
	}
	return 0;

	/*fast_io::async_server server(5002, fast_io::sock::type::stream);
	fast_io::epoll::handle_pool pool(512);
	add_control(pool, server, fast_io::epoll::event::in);
	std::array<fast_io::epoll::events, 512> events_buffer;
	std::vector<fast_io::acceptor_buf> clients;
	for (;;)
		for (auto const &ele : wait(pool, events_buffer))
			switch (get(ele))
			{
			case fast_io::epoll::event::in:
				add_control(pool, clients.emplace_back(server), fast_io::epoll::event::out | fast_io::epoll::event::hup);
				break;
			case fast_io::epoll::event::out:
			case fast_io::epoll::event::hup:
				for (auto it(clients.begin()); it != clients.end(); ++it)
					if (*it == ele)
					{
						handle_connection(*it);
						iter_swap(it, clients.end() - 1);
						clients.pop_back();
						break;
					}
			};*/
}
catch (std::exception const &e)
{
	println(fast_io::err, e);
	return 1;
}
