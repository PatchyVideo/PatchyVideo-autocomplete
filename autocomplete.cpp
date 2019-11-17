
#include "fast_io/fast_io.h"
#include "fast_io/fast_io_network.h"
#include "fast_io/fast_io_device.h"

#include <vector>
#include <string>
#include <array>
#include <queue>
#include <algorithm>
#include <memory>

template <typename T,
	typename TIter = decltype(std::begin(std::declval<T>())),
	typename = decltype(std::end(std::declval<T>()))>
	constexpr auto enumerate(T &&iterable)
{
	struct iterator
	{
		size_t i;
		TIter iter;
		bool operator != (const iterator &other) const { return iter != other.iter; }
		void operator ++ () { ++i; ++iter; }
		auto operator * () const { return std::tie(i, *iter); }
	};
	struct iterable_wrapper
	{
		T iterable;
		auto begin() { return iterator{0, std::begin(iterable)}; }
		auto end() { return iterator{0, std::end(iterable)}; }
	};
	return iterable_wrapper{std::forward<T>(iterable)};
}

struct TrieNode
{
	union
	{
		std::uint32_t ch{0};
		char ch_[4];
	};
	std::uint32_t mask{0};
	std::uint32_t freq{0};
	TrieNode *parent{nullptr};

	std::vector<TrieNode *> alias_src;
	TrieNode *alias_dst{nullptr};

	struct ChildNode
	{
		std::uint32_t ch{0};
		std::uint32_t mask{0};
		std::unique_ptr<TrieNode> child;
	};

	std::vector<ChildNode> children;
};

std::array<std::unique_ptr<TrieNode>, 256 * 256> g_tree;
std::unordered_map<std::string, std::string> g_category_map;

auto QueryWord(std::string const &prefix, std::uint32_t max_words)
{
	std::vector<std::tuple<std::string, std::string, std::string, std::uint32_t>> ret{};
	ret.reserve(max_words);

	auto prefix_iter(prefix.cbegin());
	uint16_t root_key((static_cast<std::uint8_t>(prefix_iter[1]) << 8) | static_cast<std::uint8_t>(prefix_iter[0]));
	std::string base_word(prefix.cbegin(), prefix.cbegin() + 2);

	TrieNode *cur(g_tree[root_key].get());
	if (!cur)
		return ret;
	prefix_iter += 2;

	while (prefix.cend() - prefix_iter >= 4)
	{
		TrieNode *child{nullptr};
		std::uint32_t key((static_cast<std::uint8_t>(prefix_iter[3]) << 24) | (static_cast<std::uint8_t>(prefix_iter[2]) << 16) | (static_cast<std::uint8_t>(prefix_iter[1]) << 8) | static_cast<std::uint8_t>(prefix_iter[0]));
		for (auto const &[ch, mask, nnode] : cur->children)
		{
			if (key == ch)
			{
				child = nnode.get();
				prefix_iter += 4;
				break;
			}
		}
		if (!child)
			return ret;
		else
			cur = child;
	}

	auto cmp([](TrieNode *a, TrieNode *b) {
		return a->freq < b->freq;
	});

	std::priority_queue<TrieNode *, std::vector<TrieNode *>, decltype(cmp)> q;
	std::uint32_t key{0};
	ptrdiff_t remaining_length(prefix.cend() - prefix_iter);
	std::uint32_t mask((1u << (remaining_length * 8)) - 1u);
	for (ptrdiff_t i(0); i < remaining_length; ++i)
		key |= static_cast<std::uint8_t>(prefix_iter[i]) << (i * 8);
	for (auto const &[ch, mask_, nnode] : cur->children)
		if ((key & mask) == (ch & mask))
			q.push(nnode.get());

	auto build_str([](TrieNode *node) {
		std::string s;
		s.reserve(20);
		for (; node; node = node->parent)
		{
			auto &ch(node->ch_);
			for (std::size_t i(4); i--;)
				if (ch[i])
					s.push_back(ch[i]);
		}
		std::reverse(s.begin(), s.end());
		return s;
	});

	while (!q.empty())
	{
		auto node(q.top());
		q.pop();

		if (node->mask != 0xFFFFFFFF)
		{
			auto tag(build_str(node));
			auto category(g_category_map.at(tag));
			if (node->alias_dst)
				ret.emplace_back(tag, build_str(node->alias_dst), category, node->freq);
			else
				ret.emplace_back(tag, "", category, node->freq);
			if (ret.size() == max_words)
				return ret;
		}
		else
		{
			for (auto const &[ch, mask_, nnode] : node->children)
				q.push(nnode.get());
		}
	}

	return ret;
}

inline void BackpropFreq(TrieNode *node)
{
	for (; node; node = node->parent)
	{
		node->freq = 0;
		for (auto const &[ch, mask, nnode] : node->children)
			node->freq = std::max(node->freq, nnode->freq);
	}
}

inline void BackpropFreqLeaf(TrieNode *leaf)
{
	for (auto node(leaf->parent); node; node = node->parent)
	{
		node->freq = 0;
		for (auto const &[ch, mask, nnode] : node->children)
			node->freq = std::max(node->freq, nnode->freq);
	}
}

TrieNode *UpdateOrAddWordOrAlias(std::string const &word, std::uint32_t freq)
{
	auto word_iter(word.cbegin());
	uint16_t root_key((static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));

	TrieNode *cur(g_tree[root_key].get());
	if (!cur)
	{
		g_tree[root_key].reset(new TrieNode);
		cur = g_tree[root_key].get();
		cur->ch = root_key << 16;
	}
	word_iter += 2;

	while (word.cend() - word_iter >= 4)
	{
		TrieNode *child{nullptr};
		std::uint32_t key((static_cast<std::uint8_t>(word_iter[3]) << 24) | (static_cast<std::uint8_t>(word_iter[2]) << 16) | (static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));
		for (auto const &[ch, mask, nnode] : cur->children)
		{
			if (key == ch)
			{
				child = nnode.get();
				word_iter += 4;
				break;
			}
		}
		if (!child)
		{
			std::unique_ptr<TrieNode> nnode(new TrieNode);
			nnode->ch = key;
			nnode->mask = 0xFFFFFFFF;
			nnode->parent = cur;
			auto tmp(nnode.get());
			cur->children.push_back({nnode->ch, nnode->mask, std::move(nnode)});
			cur = tmp;
			word_iter += 4;
		}
		else
			cur = child;
	}

	std::uint32_t key{0};
	ptrdiff_t remaining_length(word.cend() - word_iter);
	std::uint32_t mask((1u << (remaining_length * 8)) - 1u);
	for (ptrdiff_t i(0); i < remaining_length; ++i)
		key |= static_cast<std::uint8_t>(word_iter[i]) << (i * 8);

	auto found(std::find_if(cur->children.begin(), cur->children.end(), [&key](auto const &a) {return a.ch == key; }));

	if (found == cur->children.end())
	{
		std::unique_ptr<TrieNode> nnode(new TrieNode);
		nnode->ch = key;
		nnode->mask = mask;
		nnode->parent = cur;
		nnode->freq = freq;
		auto tmp(nnode.get());
		cur->children.push_back({nnode->ch, nnode->mask, std::move(nnode)});
		BackpropFreqLeaf(tmp);
		return tmp;
	}
	else
	{
		if (found->child->alias_dst)
		{
			// alias
			found->child->alias_dst->freq = freq;
			for (TrieNode *&alias_src : found->child->alias_dst->alias_src)
			{
				alias_src->freq = freq;
				BackpropFreqLeaf(alias_src);
			}
		}
		else
		{
			// word
			found->child->freq = freq;
			for (TrieNode *&alias_src : found->child->alias_src)
			{
				alias_src->freq = freq;
				BackpropFreqLeaf(alias_src);
			}
		}
		BackpropFreq(cur);
		return found->child.get();
	}
}

TrieNode *FindLeafNode(std::string const &word)
{
	auto word_iter(word.cbegin());
	uint16_t root_key((static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));

	TrieNode *cur(g_tree[root_key].get());
	if (!cur)
		return nullptr;
	word_iter += 2;

	while (word.cend() - word_iter >= 4)
	{
		TrieNode *child{nullptr};
		std::uint32_t key((static_cast<std::uint8_t>(word_iter[3]) << 24) | (static_cast<std::uint8_t>(word_iter[2]) << 16) | (static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));
		for (auto const &[ch, mask, nnode] : cur->children)
		{
			if (key == ch)
			{
				child = nnode.get();
				word_iter += 4;
				break;
			}
		}
		if (!child)
			return nullptr;
		else
			cur = child;
	}

	std::uint32_t key{0};
	ptrdiff_t remaining_length(word.cend() - word_iter);
	std::uint32_t mask((1u << (remaining_length * 8)) - 1u);
	for (ptrdiff_t i(0); i < remaining_length; ++i)
		key |= static_cast<std::uint8_t>(word_iter[i]) << (i * 8);

	auto found(std::find_if(cur->children.begin(), cur->children.end(), [&key](auto const &a) {return a.ch == key; }));
	if (found == cur->children.end())
		return nullptr;
	return found->child.get();
}

void MakeOrAddWordAlias(std::string const &src, std::string const &dst)
{
	auto src_leaf(FindLeafNode(src));
	auto dst_leaf(FindLeafNode(dst));

	if (!dst_leaf)
		throw std::runtime_error("dst not found");

	g_category_map[src] = g_category_map.at(dst);

	// if dst is also alias, find the root word
	while (dst_leaf->alias_dst)
		dst_leaf = dst_leaf->alias_dst;

	if (!src_leaf)
		src_leaf = UpdateOrAddWordOrAlias(src, dst_leaf->freq);

	dst_leaf->alias_src.emplace_back(src_leaf);
	src_leaf->alias_dst = dst_leaf;
	src_leaf->freq = dst_leaf->freq;

	BackpropFreqLeaf(src_leaf);
}


void DeleteAlias(std::string const &word)
{
	auto cur(FindLeafNode(word));
	if (!cur)
		throw std::runtime_error("word not found");

	if (!cur->alias_dst)
		throw std::runtime_error("not an alias");

	auto leaf(cur);
	auto &list_of_src(leaf->alias_dst->alias_src);

	g_category_map.erase(word);

	list_of_src.erase(std::remove_if(list_of_src.begin(), list_of_src.end(), [&leaf](auto const &src) {return src == leaf; }), list_of_src.end());

	auto key(cur->ch);
	for (cur = cur->parent; cur; cur = cur->parent)
	{
		auto found = std::find_if(cur->children.begin(), cur->children.end(), [&key](auto const &a) {return a.ch == key; });
		cur->children.erase(found);
		if (cur->children.size() > 0)
			break;
		key = cur->ch;
	}

	BackpropFreq(cur);
}

void DeleteAliasLink(std::string const &src)
{
	auto cur(FindLeafNode(src));
	if (!cur)
		throw std::runtime_error("word not found");

	if (!cur->alias_dst)
		throw std::runtime_error("not an alias");

	auto leaf(cur);
	auto &list_of_src(leaf->alias_dst->alias_src);

	list_of_src.erase(std::remove_if(list_of_src.begin(), list_of_src.end(), [&leaf](auto const &src) {return src == leaf; }), list_of_src.end());
	cur->alias_dst = nullptr;
	cur->freq = 0;

	BackpropFreq(cur);
}

void DeleteWord(std::string const &word)
{
	auto cur(FindLeafNode(word));
	if (!cur)
		throw std::runtime_error("word not found");

	if (cur->alias_dst)
		throw std::runtime_error("not a word");

	g_category_map.erase(word);

	for (TrieNode *&src : cur->alias_src)
	{
		src->alias_dst = nullptr;
		src->freq = 0;
	}

	auto key(cur->ch);
	for (cur = cur->parent; cur; cur = cur->parent)
	{
		auto found = std::find_if(cur->children.begin(), cur->children.end(), [&key](auto const &a) {return a.ch == key; });
		cur->children.erase(found);
		if (cur->children.size() > 0)
			break;
		key = cur->ch;
	}
	BackpropFreq(cur);
}

void DeleteWordOrAlias(std::string const &word)
{
	auto cur(FindLeafNode(word));
	if (!cur)
		throw std::runtime_error("word not found");

	if (cur->alias_dst)
		// this is an alisa item
		DeleteAlias(word);
	else
		// this is a word item
		DeleteWord(word);

}

void AddWord(std::string const &word, std::string const &category, std::uint32_t freq)
{
	g_category_map[word] = category;

	auto word_iter(word.cbegin());
	uint16_t root_key((static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));

	TrieNode *cur(g_tree[root_key].get());
	if (!cur)
	{
		g_tree[root_key].reset(new TrieNode);
		cur = g_tree[root_key].get();
		cur->ch = root_key << 16;
	}
	word_iter += 2;

	while (word.cend() - word_iter >= 4)
	{
		TrieNode *child{nullptr};
		std::uint32_t key((static_cast<std::uint8_t>(word_iter[3]) << 24) | (static_cast<std::uint8_t>(word_iter[2]) << 16) | (static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));
		for (auto const &[ch, mask, nnode] : cur->children)
		{
			if (key == ch)
			{
				child = nnode.get();
				word_iter += 4;
				break;
			}
		}
		if (!child)
		{
			std::unique_ptr<TrieNode> nnode(new TrieNode);
			nnode->ch = key;
			nnode->mask = 0xFFFFFFFF;
			nnode->parent = cur;
			auto tmp(nnode.get());
			cur->children.push_back({nnode->ch, nnode->mask, std::move(nnode)});
			cur = tmp;
			word_iter += 4;
		}
		else
			cur = child;
	}

	std::uint32_t key{0};
	ptrdiff_t remaining_length(word.cend() - word_iter);
	std::uint32_t mask((1u << (remaining_length * 8)) - 1u);
	for (ptrdiff_t i(0); i < remaining_length; ++i)
		key |= static_cast<std::uint8_t>(word_iter[i]) << (i * 8);

	std::unique_ptr<TrieNode> nnode(new TrieNode);
	nnode->ch = key;
	nnode->mask = mask;
	nnode->parent = cur;
	nnode->freq = freq;
	cur->children.push_back({nnode->ch, nnode->mask, std::move(nnode)});

	for (auto node(cur); node; node = node->parent)
	{
		node->freq = 0;
		for (auto const &[ch, mask, nnode] : node->children)
			node->freq = std::max(node->freq, nnode->freq);
	}
}

template<std::size_t x>
struct PrintSizeof;

void print_result(std::vector<std::tuple<std::string, std::string, std::uint32_t>> const &ret)
{
	for (auto const &[src, dst, freq] : ret)
	{
		if (dst.size() > 0)
			println(fast_io::out, src, " -> ", dst, "\t", freq);
		else
			println(fast_io::out, src, "\t\t", freq);
	}
}


inline constexpr std::uint64_t hash(std::string_view str)
{
	std::uint64_t ret = 5381;

	for (auto const &ch : str)
		ret = ((ret << 5) + ret) + ch; /* hash * 33 + c */

	return ret;
}

auto response_header{"HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\nContent-Length: "};

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
	if (prefix.size() < 2)
		abort(400);
	std::uint32_t max_words{10};
	fast_io::istring_view isv(max_words_str);
	fast_io::scan(isv, max_words);
	if (max_words > 100 || max_words == 0)
		max_words = 10;

	//std::vector<std::tuple<std::string, std::string, std::uint32_t>>
	auto query_result(QueryWord(prefix, max_words));
	print(out, "[");
	for (std::size_t i(0); i != query_result.size(); ++i)
	{
		auto const &[src, dst, category, freq] = query_result[i];
		print(out, "{\"src\":\"", src, "\",\"dst\":\"", dst, "\",\"category\":\"", category, "\",\"count\":", freq, "}");
		if (i != query_result.size() - 1)
			print(out, ",");
	}
	print(out, "]");
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_addwords(output &out, input &content)
{
	std::string word, cat;
	std::uint32_t num{0};
	std::size_t n(0);
	fast_io::scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		fast_io::scan(content, word, cat, num);
		if (word.size() < 2)
			return;

		AddWord(word, cat, num);
	}
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_addalias(output &out, input &content)
{
	std::string src, dst;
	std::size_t n(0);
	fast_io::scan(content, n);
	for (std::size_t i(0); i != n; ++i)
	{
		fast_io::scan(content, src, dst);
		if (src.size() < 2 || dst.size() < 2)
			return;

		MakeOrAddWordAlias(src, dst);
	}
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_setword(output &out, input &content)
{
	std::string word;
	std::uint32_t num{0};
	fast_io::scan(content, word, num);
	if (word.size() < 2)
		return;

	UpdateOrAddWordOrAlias(word, num);
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_delword(output &out, input &content)
{
	std::string word;
	fast_io::scan(content, word);
	if (word.size() < 2)
		return;

	DeleteWordOrAlias(word);
}

template<fast_io::character_output_stream output, fast_io::character_input_stream input>
inline void handle_request_delalias(output &out, input &content)
{
	std::string src;
	fast_io::scan(content, src);
	if (src.size() < 2)
		return;

	DeleteAliasLink(src);
}

/*
*   POST /addwords    n word cat num ...  return "" // must be called before POST /bulkalias
*   POST /addalias    n src dst ...       return ""
*   POST /setword     word freq           return "" // works on both word/alias
*   POST /delword     word                return "" // works on both word/alias
*   POST /delalias    src                 return "" // remove alias link, not deleting
*   GET  /?q=<prefix>&n=<max_words>       return JSON[{src,dst,freq},...]
*/


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
	case hash("/addwords"):
	handle_request_addwords(response_body_stream, content);
	break;
	case hash("/addalias"):
	handle_request_addalias(response_body_stream, content);
	break;
	case hash("/setword"):
	handle_request_setword(response_body_stream, content);
	break;
	case hash("/delword"):
	handle_request_delword(response_body_stream, content);
	break;
	case hash("/delalias"):
	handle_request_delalias(response_body_stream, content);
	break;
	default:
	abort(404);
	}
	break;
	}

	print(out, response_header);
	print(out, response_body_stream.str().size());
	print(out, "\n\n");
	print(out, response_body_stream.str());
}

int main()
try
{
	fast_io::server hd(8080, fast_io::sock::type::stream);
	for (;;)
		try
	{
		fast_io::acceptor_buf client_stream(hd);
		try
		{
			auto request_header(scan_http_header(client_stream));
			RequestMethod method;
			std::string path_version;
			if (request_header.count("GET"))
			{
				method = RequestMethod::GET;
				path_version = request_header["GET"];
			}
			else if (request_header.count("POST"))
			{
				method = RequestMethod::POST;
				path_version = request_header["POST"];
			}
			else
			{
				print(client_stream, response405);
				continue;
			}

			std::string raw_path;
			fast_io::istring_view isv(path_version);
			fast_io::scan(isv, raw_path);

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
