#include <memory>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <queue>
#include <set>
#include <bitset>

template<typename T>
struct PrintSizeof
{
	template<std::size_t size>
	struct PrintSizeof_impl;
	PrintSizeof()
	{
		PrintSizeof_impl<sizeof(T)> a;
	}
};

struct Keyword;

template<typename T, std::size_t chunk_size = 1024, std::size_t init_chunks = 1024>
struct MemoryPool
{
	std::vector<void *> pools;
	std::size_t num_of_allocated_elements_in_current_chunk;
	void *next_free_element;
	MemoryPool() : num_of_allocated_elements_in_current_chunk(0), next_free_element(nullptr)
	{
		pools.reserve(init_chunks);
		allocate_chunk();
	}
	MemoryPool(MemoryPool const &a) = delete;
	MemoryPool(MemoryPool &&a) = delete;
	MemoryPool &operator=(MemoryPool const &a) = delete;
	MemoryPool &operator=(MemoryPool &&a) = delete;
	~MemoryPool() noexcept
	{
		try
		{
			for (std::size_t i(0); i < pools.size(); ++i)
			{
				void *ptr(pools[i]);
				T *all_elements(static_cast<T *>(ptr));
				std::size_t chunk_length((i == pools.size() - 1) ? num_of_allocated_elements_in_current_chunk : chunk_size);
				for (std::size_t j(0); j < chunk_length; ++j)
				{
					all_elements[j].~T();
				}
				delete[] static_cast<char *>(ptr);
			}
		}
		catch (...)
		{
			;
		}
	}
	void allocate_chunk()
	{
		std::size_t block_size(sizeof(T) * chunk_size);
		void *new_chunk = static_cast<void *>(new char[block_size]);
		num_of_allocated_elements_in_current_chunk = 0;
		next_free_element = new_chunk;
		pools.emplace_back(new_chunk);
	}
	void *allocate(size_t size)
	{
		auto ret(next_free_element);
		if (++num_of_allocated_elements_in_current_chunk == chunk_size)
			allocate_chunk();
		else [[likely]]
			next_free_element = static_cast<void *>(static_cast<char *>(next_free_element) + sizeof(T));
		return ret;
	}
	void deallocate(void *ptr) {
		; // this happens so rarely that I don't even care about it
	}
};

struct TrieNode
{
	Keyword *keyword{nullptr};
	std::uint32_t count{0};

	union
	{
		std::uint32_t ch{0};
		char ch_[4];
	};
	std::uint32_t mask{0};

	TrieNode *parent{nullptr};

	struct ChildNode
	{
		std::uint32_t ch{0};
		std::uint32_t mask{0};
		TrieNode *child;
	};

	std::vector<ChildNode> children;
};

using TrieNodeMemoryPool = MemoryPool<TrieNode>;

void *operator new(std::size_t sz, TrieNodeMemoryPool &pool)
{
	return pool.allocate(sz);
}

void operator delete(void *ptr, TrieNodeMemoryPool &pool)
{
	pool.deallocate(ptr);
}

static TrieNodeMemoryPool g_trienodePool;

std::array<TrieNode *, 256> g_querywords;

void InitRootTrieNodes()
{
	std::uint32_t i = 0;
	for (auto &ele : g_querywords)
	{
		ele = new(g_trienodePool) TrieNode;
		ele->ch = (i++) << 24;
	}
}

struct Keyword
{
	std::uint32_t tagid;
	std::string keyword;
	std::vector<TrieNode *> query_words;
};

std::unordered_map<std::string, std::unique_ptr<Keyword>> g_keywords;

struct Tag
{
	std::uint32_t id;
	std::uint32_t count;
	std::uint32_t category;
	std::unordered_map<std::uint32_t, Keyword *> lang_keywords;
	std::vector<Keyword *> alias_keywords;
};

std::vector<std::unique_ptr<Tag>> g_tags;

void AddTag(std::uint32_t id, std::uint32_t count, std::uint32_t category)
{
	if (g_tags.size() <= id)
	{
		std::size_t shortage(id - g_tags.size() + 1);
		for (std::size_t i(0); i < shortage; ++i)
			g_tags.emplace_back(nullptr);
	}
	g_tags[id].reset(new Tag{id, count, category});
}

void UpdateTagCategory(std::uint32_t id, std::uint32_t category)
{
	g_tags[id]->category = category;
}

void DeleteKeyword(std::string const &keyword);

void DeleteTag(std::uint32_t id)
{
	// remove all keywords
	std::vector<std::string> tmp;
	for (auto const& kw : g_tags[id].get()->lang_keywords)
		tmp.emplace_back(kw.second->keyword);
	for (auto const &kw : g_tags[id].get()->alias_keywords)
		tmp.emplace_back(kw->keyword);
	for (auto &kw : tmp)
		DeleteKeyword(kw);
	g_tags[id].reset(nullptr);
}

inline void BackpropFreq(TrieNode *node)
{
	for (; node; node = node->parent)
	{
		node->count = 0;
		for (auto const &[ch, mask, nnode] : node->children)
			node->count = std::max(node->count, nnode->count);
	}
}

inline void BackpropFreqLeaf(TrieNode *leaf)
{
	for (auto node(leaf->parent); node; node = node->parent)
	{
		node->count = 0;
		for (auto const &[ch, mask, nnode] : node->children)
			node->count = std::max(node->count, nnode->count);
	}
}

auto AddQueryWord(Keyword *keyword, std::string const &word)
{
	auto const &tag(g_tags[keyword->tagid]);
	auto word_iter(word.cbegin());
	std::uint8_t root_key(static_cast<std::uint8_t>(word_iter[0]));

	TrieNode *cur(g_querywords[root_key]);
	/*if (!cur)
	{
		g_querywords[root_key] = new(g_trienodePool) TrieNode;
		cur = g_querywords[root_key];
		cur->ch = root_key << 24;
	}*/
	word_iter += 1;

	while (word.cend() - word_iter >= 4)
	{
		TrieNode *child{nullptr};
		std::uint32_t key((static_cast<std::uint8_t>(word_iter[3]) << 24) | (static_cast<std::uint8_t>(word_iter[2]) << 16) | (static_cast<std::uint8_t>(word_iter[1]) << 8) | static_cast<std::uint8_t>(word_iter[0]));
		for (auto const &[ch, mask, nnode] : cur->children)
		{
			if (key == ch)
			{
				child = nnode;
				word_iter += 4;
				break;
			}
		}
		if (!child)
		{
			TrieNode *nnode(new(g_trienodePool) TrieNode);
			nnode->ch = key;
			nnode->mask = 0xFFFFFFFF;
			nnode->parent = cur;
			cur->children.push_back({nnode->ch, nnode->mask, nnode});
			cur = nnode;
			word_iter += 4;
		}
		else
			cur = child;
	}

	std::ptrdiff_t remaining_length(word.cend() - word_iter);

	if (remaining_length > 0)
	{
		std::uint32_t key{0};
		std::uint32_t mask((1u << (remaining_length * 8)) - 1u);
		for (std::ptrdiff_t i(0); i < remaining_length; ++i)
			key |= static_cast<std::uint8_t>(word_iter[i]) << (i * 8);


		auto found(std::find_if(cur->children.begin(), cur->children.end(), [&key](auto const &a) {return a.ch == key; }));

		if (found == cur->children.end())
		{
			// node for partial query word not exist
			// add one
			TrieNode *nnode(new(g_trienodePool) TrieNode);
			nnode->ch = key;
			nnode->mask = mask;
			nnode->parent = cur;
			nnode->count = tag->count;
			cur->children.push_back({nnode->ch, nnode->mask, nnode});
			cur = nnode;
		}
		else
			cur = found->child;
	}

	auto keyword_already_exist(std::find_if(cur->children.begin(), cur->children.end(), [&keyword](auto const &a) {return a.child->keyword == keyword; }));

	if (keyword_already_exist == cur->children.end()) [[likely]]
	{
		// keyword leaf not exist
		// add keyword leaf
		TrieNode * nnode(new(g_trienodePool) TrieNode);
		nnode->ch = 0;
		nnode->mask = 0;
		nnode->parent = cur;
		nnode->count = tag->count;
		nnode->keyword = keyword;
		cur->children.push_back({nnode->ch, nnode->mask, nnode});
		BackpropFreqLeaf(nnode);
		return nnode;
	}
	else
	{
		// keyword leaf already exist
		// ignore
		return keyword_already_exist->child;
	}

}

void DeleteQueryWord(TrieNode *leaf)
{
	auto keyword(leaf->keyword);
	auto cur(leaf->parent);
	auto found(std::find_if(cur->children.begin(), cur->children.end(), [&keyword](auto const &a) {return a.child->keyword == keyword; }));
	cur->children.erase(found);
	if (cur->children.size() == 0)
	{
		// this node is no longer being referenced by any keywords
		auto key(cur->ch);
		for (cur = cur->parent; cur; cur = cur->parent)
		{
			auto found(std::find_if(cur->children.begin(), cur->children.end(), [&key](auto const &a) {return a.ch == key; }));
			cur->children.erase(found);
			if (cur->children.size() > 0)
				break;
			key = cur->ch;
		}
	}
	BackpropFreq(cur);
}

std::vector<std::string> get_all_suffix(std::string const &keyword);

void AddKeyword(std::uint32_t tagid, std::string const &keyword, std::uint32_t lang = 0)
{
	if (g_keywords.contains(keyword))
	{
		auto const &keyword_obj(g_keywords[keyword]);
		auto &tag_obj(*g_tags[tagid]);
		if (lang != 0)
			tag_obj.lang_keywords[lang] = keyword_obj.get();
		return;
	}
	auto const suffix(get_all_suffix(keyword));
	std::unique_ptr<Keyword> keyword_obj(new Keyword{tagid, keyword});
	for (auto const &query : suffix)
	{
		auto trie_node_ptr(AddQueryWord(keyword_obj.get(), query));
		keyword_obj->query_words.emplace_back(trie_node_ptr);
	}
	auto &tag_obj(*g_tags[tagid]);
	if (lang == 0)
		tag_obj.alias_keywords.emplace_back(keyword_obj.get());
	else
		tag_obj.lang_keywords[lang] = keyword_obj.get();
	g_keywords[keyword] = std::move(keyword_obj);
}

void DeleteKeyword(std::string const &keyword)
{
	//if (g_keywords.count(keyword) == 0)
	//	return;
	auto keyword_obj(g_keywords[keyword].get());
	auto tag_obj(g_tags[keyword_obj->tagid].get());

	// remove from tag object
	auto &tag_lang_keywords(tag_obj->lang_keywords);
	auto lang_it(std::find_if(tag_lang_keywords.begin(), tag_lang_keywords.end(), [&keyword_obj](auto const &a) {return a.second == keyword_obj; }));
	while (lang_it != tag_lang_keywords.end())
	{
		tag_lang_keywords.erase(lang_it);
		lang_it = std::find_if(tag_lang_keywords.begin(), tag_lang_keywords.end(), [&keyword_obj](auto const &a) {return a.second == keyword_obj; });
	}

	auto &tag_alias_keywords(tag_obj->alias_keywords);
	auto alias_it(std::find_if(tag_alias_keywords.begin(), tag_alias_keywords.end(), [&keyword_obj](auto const &a) {return a == keyword_obj; }));
	if (alias_it != tag_alias_keywords.end())
		tag_alias_keywords.erase(alias_it);
	// remove all its query words
	for (TrieNode *leaf : keyword_obj->query_words)
	{
		DeleteQueryWord(leaf);
	}
	// remove
	g_keywords.erase(keyword);
}

void UpdateKeyword(Keyword *keyword, std::uint32_t count)
{
	for (TrieNode *leaf : keyword->query_words)
	{
		leaf->count = count;
		BackpropFreqLeaf(leaf);
	}
}

void UpdateTagCount(std::uint32_t id, std::uint32_t count)
{
	auto &tag_obj(*g_tags[id]);
	tag_obj.count = count;
	for (Keyword *kwd : tag_obj.alias_keywords)
		UpdateKeyword(kwd, count);
	for (auto const& kwd : tag_obj.lang_keywords)
		UpdateKeyword(kwd.second, count);
}

void UpdateTagCountDiff(std::uint32_t id, std::int32_t diff)
{
	auto &tag_obj(*g_tags[id]);
	tag_obj.count += diff;
	for (Keyword *kwd : tag_obj.alias_keywords)
		UpdateKeyword(kwd, tag_obj.count);
	for (auto const &kwd : tag_obj.lang_keywords)
		UpdateKeyword(kwd.second, tag_obj.count);
}

/*
VALID_LANGUAGES = {
	"NAL": "Not A Language (Alias)",
	"CHS": "Chinese (Simplified)",
	"CHT": "Chinese (Traditional)",
	"CSY": "Czech",
	"NLD": "Dutch",
	"ENG": "English",
	"FRA": "French",
	"DEU": "German",
	"HUN": "Hungarian",
	"ITA": "Italian",
	"JPN": "Japanese",
	"KOR": "Korean",
	"PLK": "Polish",
	"PTB": "Portuguese (Brazil)",
	"ROM": "Romanian",
	"RUS": "Russian",
	"ESP": "Spanish",
	"TRK": "Turkish",
	"VIN": "Vietnamese"
}
PREFERRED_LANGUAGE_MAP = {
	'CHS': ['CHS', 'CHT', 'JPN', 'ENG'],
	'CHT': ['CHT', 'JPN', 'CHS', 'ENG'],
	'JPN': ['JPN', 'CHT', 'ENG', 'CHS'],
	'ENG': ['ENG', 'JPN']
}
*/

std::vector<std::string> const g_supported_languages = {
	"NAL",
	"CHS",
	"CHT",
	"CSY",
	"NLD",
	"ENG",
	"FRA",
	"DEU",
	"HUN",
	"ITA",
	"JPN",
	"KOR",
	"PLK",
	"PTB",
	"ROM",
	"RUS",
	"ESP",
	"TRK",
	"VIN"
};

std::uint32_t GetLanguageIndex(std::string const &str)
{
	for (std::uint32_t i(0); i < g_supported_languages.size(); ++i)
		if (g_supported_languages[i] == str)
			return i;
	return 0;
}

std::uint32_t operator ""_lang(char const *s, std::size_t len)
{
	std::string str;
	str.assign(s, len);
	return GetLanguageIndex(str);
}

std::vector<std::vector<std::uint32_t>> const g_lang_perference = {
	{"NAL"_lang},
	{"CHS"_lang, "CHT"_lang, "JPN"_lang, "ENG"_lang}, // CHS
	{"CHT"_lang, "JPN"_lang, "CHS"_lang, "ENG"_lang}, // CHT
	{"CSY"_lang},
	{"NLD"_lang},
	{"ENG"_lang, "JPN"_lang}, // ENG
	{"FRA"_lang},
	{"DEU"_lang},
	{"HUN"_lang},
	{"ITA"_lang},
	{"JPN"_lang, "CHT"_lang, "ENG"_lang, "CHS"_lang}, // JPN
	{"KOR"_lang},
	{"PLK"_lang},
	{"PTB"_lang},
	{"ROM"_lang},
	{"RUS"_lang},
	{"ESP"_lang},
	{"TRK"_lang},
	{"VIN"_lang}
};

auto QueryWord(std::string const &prefix, std::uint32_t max_words, std::uint32_t user_language = 0)
{
	user_language = std::min(user_language, static_cast<std::uint32_t>(g_lang_perference.size()) - 1);
	std::vector<Keyword *> ret{};
	std::set<std::uint32_t> used_tags;
	ret.reserve(max_words);

	auto prefix_iter(prefix.cbegin());
	uint8_t root_key(static_cast<std::uint8_t>(prefix_iter[0]));

	TrieNode *cur(g_querywords[root_key]);
	/*if (!cur)
		return ret;*/
	prefix_iter += 1;

	while (prefix.cend() - prefix_iter >= 4)
	{
		TrieNode *child{nullptr};
		std::uint32_t key((static_cast<std::uint8_t>(prefix_iter[3]) << 24) | (static_cast<std::uint8_t>(prefix_iter[2]) << 16) | (static_cast<std::uint8_t>(prefix_iter[1]) << 8) | static_cast<std::uint8_t>(prefix_iter[0]));
		for (auto const &[ch, mask, nnode] : cur->children)
		{
			if (key == ch)
			{
				child = nnode;
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
		return a->count < b->count;
	});

	std::priority_queue<TrieNode *, std::vector<TrieNode *>, decltype(cmp)> q;
	std::uint32_t key{0};
	ptrdiff_t remaining_length(prefix.cend() - prefix_iter);
	std::uint32_t mask((1u << (remaining_length * 8)) - 1u);
	for (ptrdiff_t i(0); i < remaining_length; ++i)
		key |= static_cast<std::uint8_t>(prefix_iter[i]) << (i * 8);
	for (auto const &[ch, mask_, nnode] : cur->children)
		if ((key & mask) == (ch & mask))
			q.push(nnode);

	while (!q.empty())
	{
		auto node(q.top());
		q.pop();

		if (node->keyword)
		{
			// we are at leaf
			if (used_tags.count(node->keyword->tagid) > 0)
				continue;
			if (user_language == 0)
			{
				ret.emplace_back(node->keyword);
				used_tags.emplace(node->keyword->tagid);
			}
			else [[likely]]
			{
				auto const &preference(g_lang_perference[user_language]);
				auto const &tag_obj(g_tags[node->keyword->tagid]);
				bool found(false);
				for (std::uint32_t ul : preference)
				{
					if (tag_obj->lang_keywords.contains(ul))
					{
						auto const &keword_obj(tag_obj->lang_keywords[ul]);
						std::string const &keyword(keword_obj->keyword);
						if (keyword.find(prefix) != std::string::npos)
						{
							ret.emplace_back(keword_obj);
							used_tags.emplace(node->keyword->tagid);
							found = true;
							break;
						}
					}
				}
				if (!found)
				{
					ret.emplace_back(node->keyword);
					used_tags.emplace(node->keyword->tagid);
				}
			}
			if (ret.size() == max_words)
				return ret;
		}
		else
		{
			for (auto const &[ch, mask_, nnode] : node->children)
				q.push(nnode);
		}
	}

	return ret;
}


std::vector<std::string> get_all_suffix(std::string const &keyword)
{
	std::vector<std::string> ret{};
	ret.reserve(keyword.size());
	for (auto it(keyword.begin()); it != keyword.end(); ++it)
	{
		ret.emplace_back(it, keyword.end());
		union
		{
			char ch;
			std::bitset<8> bts;
		} u{*it};
		if (u.bts.test(7))
		{
			// UTF-8 char
			std::size_t ones(0);
			for (std::size_t i(6); i >= 4; --i)
				if (u.bts.test(i))
					++ones;
			it += ones;
		}
	}
	return ret;
}
