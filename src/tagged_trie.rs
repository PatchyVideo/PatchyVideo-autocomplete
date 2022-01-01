use itertools::Itertools;

use cedarwood::Cedar;

use crate::db::Tag;
use crate::schema::Languages;

use anyhow::{anyhow, Result};
use std::collections::HashMap;
use tracing::instrument;

#[derive(Debug)]
pub struct IdLangPair(u32, Languages);

#[derive(Debug)]
pub struct IdLangPairBits(i32);

impl From<IdLangPairBits> for IdLangPair {
    fn from(IdLangPairBits(num): IdLangPairBits) -> Self {
        let bits = num as u32;
        let lang_bits = bits & 0b11111;
        let id_bits = bits >> 5;
        IdLangPair(id_bits, Languages::try_from(lang_bits).unwrap())
    }
}

impl From<IdLangPair> for IdLangPairBits {
    fn from(pair: IdLangPair) -> Self {
        let IdLangPair(id_bits, lang) = pair;
        IdLangPairBits(((id_bits << 5) | u32::from(lang)) as i32)
    }
}

#[derive(Debug)]
pub struct TaggedTrie {
    pub tags: HashMap<u32, Tag>,
    pub tries: Cedar,
    pub keywords: HashMap<String, u32>,
}

impl TaggedTrie {
    pub fn build_with_tags(raw_tags: &[Tag]) -> Self {
        let tags: HashMap<u32, Tag> = raw_tags
            .iter()
            .map(|raw_tag| (raw_tag.id, raw_tag.clone()))
            .collect();

        let keywords_pair: Vec<(String, i32)> = raw_tags
            .iter()
            .flat_map(|raw_tag| {
                let keyword_mapping = raw_tag.languages.iter().flat_map(move |(lang, keyword)| {
                    let id_bit = IdLangPairBits::from(IdLangPair(raw_tag.id, *lang)).0;
                    Self::get_all_suffix(keyword)
                        .map(move |suffix| (Self::encode_prefix(suffix, raw_tag.id), id_bit))
                });
                let alias_mapping = raw_tag.alias.iter().flat_map(move |alias_word| {
                    let id_bit = IdLangPairBits::from(IdLangPair(raw_tag.id, Languages::NAL)).0;

                    Self::get_all_suffix(alias_word)
                        .map(move |suffix| (Self::encode_prefix(suffix, raw_tag.id), id_bit))
                });
                keyword_mapping.chain(alias_mapping)
            })
            // Make all the keyword to be lowercase for better search experience
            .map(|(keyword, id_bits)| (keyword.to_ascii_lowercase(), id_bits))
            .sorted_unstable()
            .collect();

        let mut cedar = Cedar::new();
        cedar.build(
            &keywords_pair
                .iter()
                .map(|(keyword, id_bits)| (keyword.as_str(), *id_bits))
                .collect::<Vec<_>>(),
        );

        let keywords = raw_tags
            .iter()
            .flat_map(|raw_tag| {
                let keyword_mapping = raw_tag
                    .languages
                    .iter()
                    .map(|(_, keyword)| (keyword.to_owned(), raw_tag.id));
                let alias_mapping = raw_tag
                    .alias
                    .iter()
                    .map(|alias_word| (alias_word.to_owned(), raw_tag.id));
                keyword_mapping.chain(alias_mapping)
            })
            .collect::<HashMap<String, u32>>();

        Self {
            tags,
            tries: cedar,
            keywords,
        }
    }

    #[instrument]
    pub fn find(&self, prefix: &str) -> Option<Vec<(Languages, Tag)>> {
        let res: Vec<(Languages, Tag)> = self
            .tries
            .common_prefix_predict(&prefix.to_ascii_lowercase())? // make keyword to be lowercase for better experience
            .iter()
            .map(|(id_bits, _)| IdLangPair::from(IdLangPairBits(*id_bits)))
            .unique_by(|&IdLangPair(tag_id, _)| tag_id)
            .filter_map(|IdLangPair(tag_id, lang)| {
                let tag = self.tags.get(&tag_id)?.clone();
                Some((lang, tag))
            })
            .collect();
        Some(res)
    }

    #[instrument]
    pub fn add_tags(&mut self, tags: &[Tag]) -> Result<()> {
        for tag in tags {
            self.add_tag(tag)?;
        }
        Ok(())
    }

    #[instrument]
    pub fn add_tag(&mut self, tag: &Tag) -> Result<()> {
        if self.tags.contains_key(&tag.id) {
            return Err(anyhow!("Tag already exists, tag id: {}", tag.id));
        }
        let keyword_mapping = tag.languages.iter().flat_map(move |(lang, keyword)| {
            let id_bit = IdLangPairBits::from(IdLangPair(tag.id, *lang)).0;
            Self::get_all_suffix(keyword)
                .map(move |suffix| (Self::encode_prefix(suffix, tag.id), id_bit))
        });
        let alias_mapping = tag.alias.iter().flat_map(move |alias_word| {
            let id_bit = IdLangPairBits::from(IdLangPair(tag.id, Languages::NAL)).0;

            Self::get_all_suffix(alias_word)
                .map(move |suffix| (Self::encode_prefix(suffix, tag.id), id_bit))
        });

        let keywords = keyword_mapping.chain(alias_mapping);

        for (keyword, id_bits) in keywords {
            self.tries.update(&keyword.to_ascii_lowercase(), id_bits);
        }

        self.tags.insert(tag.id, tag.clone());

        for (_, keyword) in tag.languages.iter() {
            self.keywords.insert(keyword.to_owned(), tag.id);
        }

        for alias_word in tag.alias.iter() {
            self.keywords.insert(alias_word.to_owned(), tag.id);
        }
        Ok(())
    }

    #[instrument]
    pub fn delete_tag(&mut self, tag_id: u32) -> Result<()> {
        let tag = self
            .tags
            .get(&tag_id)
            .ok_or(anyhow!("Cannot find the tag by tag id: {}", tag_id))?;

        let keyword_mapping = tag.languages.iter().flat_map(|(_, keyword)| {
            Self::get_all_suffix(&keyword).map(|suffix| (Self::encode_prefix(suffix, tag.id)))
        });
        let alias_mapping = tag.alias.iter().flat_map(|alias_word| {
            Self::get_all_suffix(alias_word).map(|suffix| (Self::encode_prefix(suffix, tag.id)))
        });

        let keywords = keyword_mapping.chain(alias_mapping).map(|keyword| keyword);

        for keyword in keywords {
            self.tries.erase(&keyword.to_ascii_lowercase());
        }

        for (_, keyword) in tag.languages.iter() {
            self.keywords.remove(keyword);
        }

        for alias_word in tag.alias.iter() {
            self.keywords.remove(alias_word);
        }

        self.tags.remove(&tag_id);

        Ok(())
    }

    #[instrument]
    pub fn delete_word(&mut self, word: &str) -> Result<()> {
        let tag_id = {
            self.keywords
                .get(word)
                .ok_or(anyhow!("Cannot find the word"))?
        };

        for suffix in Self::get_all_suffix(&word) {
            self.tries
                .erase(&Self::encode_prefix(suffix, *tag_id).to_ascii_lowercase());
        }

        self.keywords.remove(word);

        Ok(())
    }

    #[instrument]
    pub fn add_word(&mut self, tag_id: u32, word: &str, lang: &Languages) -> Result<()> {
        let id_bit = IdLangPairBits::from(IdLangPair(tag_id, *lang)).0;

        match lang {
            Languages::NAL => {
                self.tags
                    .get_mut(&tag_id)
                    .ok_or(anyhow!("Cannot found the tag with id {}", tag_id))?
                    .alias
                    .insert(word.to_owned());
            }
            _ => {
                self.tags
                    .get_mut(&tag_id)
                    .ok_or(anyhow!("Cannot found the tag with id {}", tag_id))?
                    .languages
                    .insert(*lang, word.to_owned());
            }
        };
        self.keywords.insert(word.to_owned(), tag_id);

        for suffix in Self::get_all_suffix(word) {
            self.tries.update(&suffix.to_ascii_lowercase(), id_bit);
        }

        Ok(())
    }

    fn get_all_suffix(s: &str) -> impl Iterator<Item = &str> {
        use unicode_segmentation::UnicodeSegmentation;
        s.grapheme_indices(false).map(|(start, _)| &s[start..])
    }

    // Since the same prefix will overwrite the values, we will append the tag id after the prefix to dedup
    fn encode_prefix(keyword: &str, tag_id: u32) -> String {
        format!("{}:{}", keyword, tag_id)
    }
}

#[cfg(test)]
mod tests {
    use crate::db::Tag;
    use crate::schema::Category;
    use crate::schema::Languages;
    use maplit::hashmap;
    use maplit::hashset;

    use super::TaggedTrie;

    fn setup_tags() -> Vec<Tag> {
        vec![
            Tag {
                id: 1,
                category: Category::Character,
                count: 1,
                icon: "".to_string(),
                alias: hashset!["红白".to_owned()],
                languages: hashmap! {
                    Languages::CHS => "博丽灵梦".to_owned()
                },
            },
            Tag {
                id: 2,
                category: Category::Character,
                count: 2,
                icon: "".to_string(),
                alias: hashset!["黑白".to_owned()],
                languages: hashmap! {
                    Languages::CHS => "雾雨魔理沙".to_owned(),
                    Languages::CHT => "霧雨魔理沙".to_owned(),
                    Languages::JPN => "霧雨魔理沙".to_owned(),
                },
            },
            Tag {
                id: 3,
                category: Category::Character,
                count: 3,
                icon: "".to_string(),
                alias: hashset![],
                languages: hashmap! {
                    Languages::CHS => "东风谷早苗".to_owned(),
                },
            },
            Tag {
                id: 4,
                category: Category::Copyright,
                count: 4,
                icon: "".to_owned(),
                alias: hashset!["车万".to_owned()],
                languages: hashmap! {
                    Languages::CHS => "东方".to_owned(),
                    Languages::ENG => "Touhou".to_owned(),
                },
            },
        ]
    }

    #[test]
    fn test_tagged_trie_build() {
        let tags = setup_tags();
        TaggedTrie::build_with_tags(&tags);
    }

    #[test]
    fn test_find() {
        let tags = setup_tags();
        let tagged_trie = TaggedTrie::build_with_tags(&tags);

        assert_eq!(
            tagged_trie
                .find("东")
                .unwrap()
                .iter()
                .map(|(_, tag)| { tag.id })
                .collect::<Vec<u32>>(),
            vec![4, 3]
        );
    }

    #[test]
    fn test_suffix_find() {
        let tags = setup_tags();
        let tagged_trie = TaggedTrie::build_with_tags(&tags);

        assert_eq!(
            tagged_trie
                .find("早")
                .unwrap()
                .iter()
                .map(|(_, tag)| { tag.id })
                .collect::<Vec<u32>>(),
            vec![3]
        );
    }

    #[test]
    fn test_alias_find() {
        let tags = setup_tags();
        let tagged_trie = TaggedTrie::build_with_tags(&tags);

        assert_eq!(
            tagged_trie
                .find("白")
                .unwrap()
                .iter()
                .map(|(_, tag)| { tag.id })
                .collect::<Vec<_>>(),
            vec![1, 2]
        );
    }

    #[test]
    fn test_other_languages_find() {
        let tags = setup_tags();
        let tagged_trie = TaggedTrie::build_with_tags(&tags);

        assert_eq!(
            tagged_trie
                .find("tou")
                .unwrap()
                .iter()
                .map(|(lang, tag)| { (lang, tag.id) })
                .collect::<Vec<_>>(),
            vec![(&Languages::ENG, 4)]
        );
    }
    #[test]
    fn test_add_tags() {
        let mut tagged_trie = TaggedTrie::build_with_tags(&vec![]);
        let tags = vec![
            Tag {
                id: 1,
                category: Category::Character,
                count: 1,
                icon: "".to_string(),
                alias: hashset!["红白".to_owned()],
                languages: hashmap! {
                    Languages::CHS => "博丽灵梦".to_owned()
                },
            },
            Tag {
                id: 2,
                category: Category::Character,
                count: 2,
                icon: "".to_string(),
                alias: hashset!["黑白".to_owned()],
                languages: hashmap! {
                    Languages::CHS => "雾雨魔理沙".to_owned(),
                },
            },
        ];
        tagged_trie.add_tags(&tags).unwrap();
        assert_eq!(tagged_trie.tags.len(), 2);

        assert_eq!(
            tagged_trie
                .find("博丽")
                .unwrap()
                .iter()
                .map(|(_, tag)| tag.id)
                .collect::<Vec<_>>(),
            vec![1]
        );

        assert_eq!(
            tagged_trie
                .find("雾雨")
                .unwrap()
                .iter()
                .map(|(_, tag)| tag.id)
                .collect::<Vec<_>>(),
            vec![2]
        );
    }

    #[test]
    fn test_delete_tags() {
        let tags = setup_tags();
        let mut tagged_trie = TaggedTrie::build_with_tags(&tags);
        tagged_trie.delete_tag(1).unwrap();
        assert_eq!(tagged_trie.tags.len(), 3);

        let empty_array: Vec<u32> = Vec::new();
        assert_eq!(
            tagged_trie
                .find("博丽")
                .unwrap()
                .iter()
                .map(|(_, tag)| tag.id)
                .collect::<Vec<_>>(),
            empty_array
        );

        tagged_trie.delete_tag(2).unwrap();
        assert_eq!(tagged_trie.tags.len(), 2);

        assert_eq!(
            tagged_trie
                .find("雾雨")
                .unwrap()
                .iter()
                .map(|(_, tag)| tag.id)
                .collect::<Vec<_>>(),
            empty_array
        );
    }
    #[test]
    fn test_find_with_dup() {
        let tags = setup_tags();
        let tagged_trie = TaggedTrie::build_with_tags(&tags);

        assert_eq!(
            tagged_trie
                .find("霧雨魔理沙")
                .unwrap()
                .iter()
                .map(|(lang, tag)| { (lang, tag.id) })
                .collect::<Vec<_>>(),
            vec![(&Languages::JPN, 2)]
        );
    }
}
