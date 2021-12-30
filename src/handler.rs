use axum::{
    extract::{Extension, Query},
    http::StatusCode,
    response::{IntoResponse, Json},
};
use serde_json::{json, Value};
use std::{
    collections::{HashMap, HashSet},
    str::FromStr,
    sync::Arc,
};
use tap::TapOptional;
use tracing::{error, info};

use crate::{
    config::AppContext,
    db::Tag,
    schema::{
        AddTagRequest, AddTagRequestCollection, AddWordRequest, AddWordRequestCollection,
        Languages, MatchFirstRequest, MatchFirstRequestCollection, QlResponse, RootResponse,
        SetCatRequest, SetCatRequestCollection, SetCountDiffRequest, SetCountDiffRequestCollection,
        SetCountRequest, SetCountRequestCollection,
    },
    tagged_trie::TaggedTrie,
};
use anyhow::{anyhow, Result};

pub async fn add_tag(
    AddTagRequestCollection(req): AddTagRequestCollection,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, String) {
    let tags = req
        .into_iter()
        .map(
            |AddTagRequest {
                 tagid,
                 count,
                 category,
             }| {
                Tag {
                    id: tagid,
                    category,
                    count,
                    icon: "".to_owned(),
                    alias: HashSet::new(),
                    languages: HashMap::new(),
                }
            },
        )
        .collect::<Vec<_>>();

    let mut tagged_trie = context.tagged_trie.write();

    for tag in tags.iter() {
        if tagged_trie.tags.contains_key(&tag.id) {
            return (
                StatusCode::BAD_REQUEST,
                format!("ERROR: tag id: {} already exists", tag.id),
            );
        }
    }
    match tagged_trie.add_tags(&tags) {
        Ok(()) => (StatusCode::OK, "Successfully add tags".to_owned()),
        Err(err) => (StatusCode::BAD_REQUEST, err.to_string()),
    }
}

pub async fn add_word(
    AddWordRequestCollection(reqs): AddWordRequestCollection,
    Extension(context): Extension<Arc<AppContext>>,
) -> impl IntoResponse {
    let mut tagged_trie = context.tagged_trie.write();

    if let Err(err) =
        check_tag_ids_availability(reqs.iter().map(|req| &req.tagid), &tagged_trie.tags)
    {
        return (StatusCode::BAD_REQUEST, err.to_string());
    }
    for AddWordRequest { tagid, word, lang } in reqs {
        if let Err(err) = tagged_trie.add_word(tagid, &word, &lang) {
            return (StatusCode::INTERNAL_SERVER_ERROR, err.to_string());
        }
    }
    (StatusCode::OK, "Successfully add words".to_owned())
}

pub async fn set_count(
    SetCountRequestCollection(reqs): SetCountRequestCollection,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, String) {
    let mut tagged_trie = context.tagged_trie.write();

    if let Err(err) =
        check_tag_ids_availability(reqs.iter().map(|req| &req.tagid), &tagged_trie.tags)
    {
        return (StatusCode::BAD_REQUEST, err.to_string());
    }
    for SetCountRequest { tagid, count } in reqs.into_iter() {
        let mut tag = tagged_trie.tags.get_mut(&tagid).unwrap();
        tag.count = count;
    }
    (StatusCode::OK, "Successfully update the counts".to_owned())
}

pub async fn set_count_diff(
    SetCountDiffRequestCollection(reqs): SetCountDiffRequestCollection,
    Extension(context): Extension<Arc<AppContext>>,
) -> impl IntoResponse {
    let mut tagged_trie = context.tagged_trie.write();

    if let Err(err) =
        check_tag_ids_availability(reqs.iter().map(|req| &req.tagid), &tagged_trie.tags)
    {
        return (StatusCode::BAD_REQUEST, err.to_string());
    }
    for SetCountDiffRequest { tagid, diff } in reqs.into_iter() {
        let mut tag = tagged_trie.tags.get_mut(&tagid).unwrap();
        tag.count = (tag.count as i32 + diff) as u32;
    }
    (StatusCode::OK, "Successfully update the counts".to_owned())
}

pub async fn set_cat(
    SetCatRequestCollection(reqs): SetCatRequestCollection,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, String) {
    let mut tagged_trie = context.tagged_trie.write();

    if let Err(err) =
        check_tag_ids_availability(reqs.iter().map(|req| &req.tagid), &tagged_trie.tags)
    {
        return (StatusCode::BAD_REQUEST, err.to_string());
    }
    for SetCatRequest { tagid, cat } in reqs.into_iter() {
        let mut tag = tagged_trie.tags.get_mut(&tagid).unwrap();
        tag.category = cat;
    }
    (
        StatusCode::OK,
        "Successfully update the category".to_owned(),
    )
}

pub async fn del_tag(
    req: String,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, String) {
    let tag_id = match req.parse::<u32>() {
        Ok(val) => val,
        Err(_) => return (StatusCode::BAD_REQUEST, "Invalid tag id".to_owned()),
    };
    let mut tagged_trie = context.tagged_trie.write();

    if !tagged_trie.tags.contains_key(&tag_id) {
        return (
            StatusCode::BAD_REQUEST,
            format!("Tag id {} doesn't exists", tag_id),
        );
    };

    tracing::debug!("Mark 1");

    match tagged_trie.delete_tag(tag_id) {
        Ok(_) => (StatusCode::OK, "Successfully delete the tag".to_owned()),
        Err(err) => (
            StatusCode::BAD_REQUEST,
            format!("Failed to delte the tag id: {} for {}", tag_id, err),
        ),
    }
}

pub async fn del_word(
    word: String,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, String) {
    let mut tagged_trie = context.tagged_trie.write();
    match tagged_trie.delete_word(&word) {
        Ok(_) => (StatusCode::OK, "Successfully delete the word".to_owned()),
        Err(error) => (
            StatusCode::BAD_REQUEST,
            format!("Cannot delete word, error: {}", error),
        ),
    }
}

pub async fn match_first(
    MatchFirstRequestCollection(reqs): MatchFirstRequestCollection,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, Json<Value>) {
    let tagged_trie = context.tagged_trie.read();
    let res = reqs
        .into_iter()
        .filter_map(|MatchFirstRequest { word }| {
            let max_tag_id = tagged_trie
                .find(&word)?
                .into_iter()
                .max_by_key(|(_, tag)| tag.count)?
                .1
                .id;
            Some(max_tag_id)
        })
        .collect::<Vec<_>>();
    (StatusCode::OK, Json(json!(res)))
}

pub async fn root(
    Query(params): Query<HashMap<String, String>>,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, Json<Value>) {
    let maxn = params["n"].parse::<usize>().unwrap_or(10);
    let prefix = params.get("q").tap_none(|| error!("No prefix provided"));
    if maxn == 0 || prefix.is_none() {
        return (StatusCode::OK, Json(json!(Vec::<RootResponse>::new())));
    }
    let prefix = prefix.unwrap();
    let mut tags = {
        let tagged_trie = context.tagged_trie.read();
        match find_tags_from_tagged_trie(&tagged_trie, prefix) {
            Some(tags) => tags,
            None => {
                return (StatusCode::OK, Json(json!(Vec::<RootResponse>::new())));
            }
        }
    };

    // Filtered by l
    if let Some(preferred_lang) = params.get("l") {
        let preferred_lang = Languages::from_str(preferred_lang).expect("Unsupported language");
        let fallback_preferred_lang = context
            .config
            .language_preference
            .get(&preferred_lang.to_string());
        tags = tags
            .into_iter()
            .filter(|(lang, _)| {
                lang == &preferred_lang
                    || (fallback_preferred_lang.is_some()
                        && fallback_preferred_lang.unwrap().contains(lang))
            })
            .collect::<Vec<_>>();
    }

    let mut res = tags.into_iter()
        .filter_map(|(lang, tag)| {
            let matched_keyword = match lang {
                Languages::NAL => tag.alias.iter().find(|alias|{alias.starts_with(prefix)}),
                _ => tag.languages.get(&lang).tap_none(||{ error!("Tag doesn't contain the matched prefix, prefix: {:?}, lang: {:?}, tag_id: {:?}", prefix, lang, tag.id); }),
            }?;
            Some(
                RootResponse {
                    word: matched_keyword.clone(),
                    category: tag.category,
                    count: tag.count,
                }
            )
        })
        .collect::<Vec<RootResponse>>();

    res = take_top_n_from_vec(res, maxn, |a, b| b.count.cmp(&a.count));

    res.sort_unstable_by(|a, b| b.count.cmp(&a.count));

    (StatusCode::OK, Json(json!(res)))
}

pub async fn ql(
    Query(params): Query<HashMap<String, String>>,
    Extension(context): Extension<Arc<AppContext>>,
) -> (StatusCode, Json<Value>) {
    let maxn = params["n"].parse::<usize>().unwrap_or(10);
    let prefix = params.get("q").tap_none(|| error!("No prefix provided"));
    if maxn == 0 || prefix.is_none() {
        return (StatusCode::OK, Json(json!(Vec::<QlResponse>::new())));
    }
    let prefix = prefix.unwrap();

    let tags = {
        let tagged_trie = context.tagged_trie.read();
        match find_tags_from_tagged_trie(&tagged_trie, prefix) {
            Some(tags) => tags,
            None => {
                return (StatusCode::OK, Json(json!(Vec::<RootResponse>::new())));
            }
        }
    };

    let mut res = tags.into_iter()
        .filter_map(|(lang, tag)| {
            let matched_keyword = match lang {
                Languages::NAL => tag.alias.iter().find(|alias|{alias.starts_with(prefix)}),
                _ => tag.languages.get(&lang).tap_none(||{ error!("Tag doesn't contain the matched prefix, prefix: {:?}, lang: {:?}, tag_id: {:?}", prefix, lang, tag.id); }),
            }?;
            Some(
                QlResponse {
                    category: tag.category,
                    count: tag.count,
                    matched_keyword: matched_keyword.clone(),
                    langs: tag.languages,
                    alias: tag.alias,
                }
            )
        })
        .collect::<Vec<QlResponse>>();

    res = take_top_n_from_vec(res, maxn, |a, b| b.count.cmp(&a.count));

    res.sort_unstable_by(|a, b| b.count.cmp(&a.count));

    (StatusCode::OK, Json(json!(res)))
}

fn find_tags_from_tagged_trie(
    tagged_trie: &TaggedTrie,
    prefix: &str,
) -> Option<Vec<(Languages, Tag)>> {
    tagged_trie
        .find(prefix)
        .tap_some(|tags| info!("Find {:?} tags", tags.len()))
        .tap_none(|| info!("Cannot find the matched tags, prefix: {:?}", prefix))
}

fn take_top_n_from_vec<T, F>(mut arr: Vec<T>, maxn: usize, compare: F) -> Vec<T>
where
    F: FnMut(&T, &T) -> std::cmp::Ordering,
{
    if arr.len() > maxn {
        arr.select_nth_unstable_by(maxn - 1, compare);
        arr = arr.drain(0..maxn).collect::<Vec<_>>();
    }
    arr
}

fn check_tag_ids_availability<'a, I>(tag_ids: I, tags: &HashMap<u32, Tag>) -> Result<()>
where
    I: Iterator<Item = &'a u32>,
{
    for tag_id in tag_ids {
        if !tags.contains_key(&tag_id) {
            return Err(anyhow!("NOT FOUND: tag id: {}", tag_id));
        }
    }
    Ok(())
}
