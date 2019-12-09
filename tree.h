
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <queue>
#include <set>
#include <bitset>

#include <iostream>

/*
* TODO: support multi-language
* TODO: support arena allocation
*/

template<std::size_t x>
struct PrintSizeof;

struct Keyword;

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
		std::unique_ptr<TrieNode> child;
	};

	std::vector<ChildNode> children;
};

std::array<std::unique_ptr<TrieNode>, 256> g_querywords;

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
	std::vector<Keyword *> keywords;
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
	std::unique_ptr<Tag> tag(new Tag{id, count, category});
	g_tags[id] = std::move(tag);
}

void UpdateTagCategory(std::uint32_t id, std::uint32_t category)
{
	g_tags[id]->category = category;
}

void DeleteKeyword(std::string const &keyword);

void DeleteTag(std::uint32_t id)
{
	auto const& keywords(g_tags[id].get()->keywords);
	// remove all keywords
	std::vector<std::string> tmp;
	for (auto &kw : keywords)
	{
		tmp.emplace_back(kw->keyword);
	}
	for (auto &kw : tmp)
	{
		DeleteKeyword(kw);
	}
	g_tags[id].release();
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

	TrieNode *cur(g_querywords[root_key].get());
	if (!cur)
	{
		g_querywords[root_key].reset(new TrieNode);
		cur = g_querywords[root_key].get();
		cur->ch = root_key << 24;
	}
	word_iter += 1;

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
			std::unique_ptr<TrieNode> nnode(new TrieNode);
			nnode->ch = key;
			nnode->mask = mask;
			nnode->parent = cur;
			nnode->count = tag->count;
			auto tmp(nnode.get());
			cur->children.push_back({nnode->ch, nnode->mask, std::move(nnode)});
			cur = tmp;
		}
		else
			cur = found->child.get();
	}

	auto keyword_already_exist(std::find_if(cur->children.begin(), cur->children.end(), [&keyword](auto const &a) {return a.child->keyword == keyword; }));

	if (keyword_already_exist == cur->children.end()) [[likely]]
	{
		// keyword leaf not exist
		// add keyword leaf
		std::unique_ptr<TrieNode> nnode(new TrieNode);
		nnode->ch = 0;
		nnode->mask = 0;
		nnode->parent = cur;
		nnode->count = tag->count;
		nnode->keyword = keyword;
		auto tmp(nnode.get());
		cur->children.push_back({nnode->ch, nnode->mask, std::move(nnode)});
		BackpropFreqLeaf(tmp);
		return tmp;
	}
	else
	{
		// keyword leaf already exist
		// ignore
		return keyword_already_exist->child.get();
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

std::vector<std::string> _get_all_suffix(std::string const &keyword);

void AddKeyword(std::uint32_t tagid, std::string const &keyword)
{
	if (g_keywords.count(keyword) > 0)
		return;
	auto suffix(_get_all_suffix(keyword));
	std::unique_ptr<Keyword> keyword_obj(new Keyword{tagid, keyword});
	for (auto const &query : suffix)
	{
		auto trie_node_ptr(AddQueryWord(keyword_obj.get(), query));
		keyword_obj->query_words.emplace_back(trie_node_ptr);
	}
	auto& tag_obj(*g_tags[tagid]);
	tag_obj.keywords.emplace_back(keyword_obj.get());
	g_keywords[keyword] = std::move(keyword_obj);
}

void DeleteKeyword(std::string const &keyword)
{
	//if (g_keywords.count(keyword) == 0)
	//	return;
	auto keyword_obj(g_keywords[keyword].get());
	auto tag_obj(g_tags[keyword_obj->tagid].get());
	auto &tag_keywords(tag_obj->keywords);
	// remove from tag object
	tag_keywords.erase(std::find_if(tag_keywords.begin(), tag_keywords.end(), [&keyword_obj](auto const &a) {return a == keyword_obj; }));
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
	for (Keyword *kwd : tag_obj.keywords)
	{
		UpdateKeyword(kwd, count);
	}
}

void UpdateTagCountDiff(std::uint32_t id, std::int32_t diff)
{
	auto &tag_obj(*g_tags[id]);
	tag_obj.count += diff;
	for (Keyword *kwd : tag_obj.keywords)
	{
		UpdateKeyword(kwd, tag_obj.count);
	}
}

auto QueryWord(std::string const &prefix, std::uint32_t max_words)
{
	std::vector<Keyword *> ret{};
	std::set<std::uint32_t> used_tags;
	ret.reserve(max_words);

	auto prefix_iter(prefix.cbegin());
	uint8_t root_key(static_cast<std::uint8_t>(prefix_iter[0]));

	TrieNode *cur(g_querywords[root_key].get());
	if (!cur)
		return ret;
	prefix_iter += 1;

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

		if (node->keyword)
		{
			// we are at leaf
			if (used_tags.count(node->keyword->tagid) > 0)
				continue;
			ret.emplace_back(node->keyword);
			used_tags.emplace(node->keyword->tagid);
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


std::vector<std::string> _get_all_suffix(std::string const &keyword)
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
