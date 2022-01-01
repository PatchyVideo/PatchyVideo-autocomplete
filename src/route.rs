use crate::{
    handler::{
        add_tag, add_word, del_tag, del_word, match_first, ql, root, set_cat, set_count,
        set_count_diff,
    },
    AppContext,
};
use axum::{
    routing::{get, post},
    AddExtensionLayer, Router,
};
use std::sync::Arc;
use tower_http::trace::TraceLayer;

/// 这是PatchyVideo的自动补全服务
/// 它包含了以下的API
/// POST /addtag         n tagid count cat ...    return ""
/// POST /addword        n tagid word lang ...    return ""
/// POST /setcount       n tagid count ...        return ""
/// POST /setcountdiff   n tagid diff ...         return ""
/// POST /setcat         n tagid cat ...          return ""
/// POST /deltag         tagid                    return ""
/// POST /delword        word                     return ""
/// POST /matchfirst     n words ...              return JSON[tagid,...]
/// GET  /?q=<prefix>&n=<max_words>&l=<lang>      return JSON[{word,category,count},...]
/// GET  /ql?q=<prefix>&n=<max_words>             return JSON[{category,count,matched keyword,langs:[{language,word},...],alias:[word,...]},...]

pub fn gen_router(context: Arc<AppContext>) -> Router {
    Router::new()
        .route("/addtag", post(add_tag))
        .route("/addword", post(add_word))
        .route("/setcount", post(set_count))
        .route("/setcountdiff", post(set_count_diff))
        .route("/setcat", post(set_cat))
        .route("/deltag", post(del_tag))
        .route("/delword", post(del_word))
        .route("/matchfirst", post(match_first))
        .route("/", get(root))
        .route("/ql", get(ql))
        .layer(AddExtensionLayer::new(context))
        .layer(TraceLayer::new_for_http())
}

#[cfg(test)]
mod tests {
    use crate::{
        config::{AppContext, Config},
        db::Tag,
        schema::{Category, Languages, QlResponse},
        tagged_trie::TaggedTrie,
    };
    use axum::{
        body::Body,
        http::{self, Request, StatusCode},
    };
    use itertools::Itertools;
    use maplit::{hashmap, hashset};
    use parking_lot::RwLock;
    use serde_json::{json, Value};
    use std::{sync::Arc, vec};
    use tower::ServiceExt;

    use super::gen_router;

    fn setup_context() -> Arc<AppContext> {
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
        ];
        let tagged_trie = TaggedTrie::build_with_tags(&tags);
        let config: Config =
            toml::from_str(&include_str!("../fixtures/dev.toml").to_string()).unwrap();

        Arc::new(AppContext {
            config,
            tagged_trie: RwLock::new(tagged_trie),
        })
    }

    #[tokio::test]
    async fn test_add_tag() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/addtag")
                    .body(Body::from("2 5 5 Character 6 6 Author"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        assert_eq!(tagged_trie.tags.len(), 6);
    }

    #[tokio::test]
    async fn test_add_word() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/addword")
                    .body(Body::from("2 1 Reimu ENG 2 Marisa ENG"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        assert_eq!(tagged_trie.keywords["Reimu"], 1);
        assert_eq!(tagged_trie.keywords["Marisa"], 2);
        let reimu = tagged_trie.find("Rei").unwrap();
        assert_eq!(reimu.len(), 1);
        assert_eq!(reimu[0].0, Languages::ENG);
        assert_eq!(reimu[0].1.languages[&Languages::ENG], "Reimu");

        let marisa = tagged_trie.find("Mar").unwrap();
        assert_eq!(marisa.len(), 1);
        assert_eq!(marisa[0].0, Languages::ENG);
        assert_eq!(marisa[0].1.languages[&Languages::ENG], "Marisa");
    }

    #[tokio::test]
    async fn test_set_count() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/setcount")
                    .body(Body::from("2 1 2 2 3"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        let reimu = tagged_trie.find("灵梦").unwrap();
        assert_eq!(reimu[0].1.count, 2);

        let marisa = tagged_trie.find("魔理沙").unwrap();
        assert_eq!(marisa[0].1.count, 3);
    }

    #[tokio::test]
    async fn test_set_count_diff() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/setcountdiff")
                    .body(Body::from("2 1 1 2 -1"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        let reimu = tagged_trie.find("灵梦").unwrap();
        assert_eq!(reimu[0].1.count, 2);

        let marisa = tagged_trie.find("魔理沙").unwrap();
        assert_eq!(marisa[0].1.count, 1);
    }

    #[tokio::test]
    async fn test_set_category() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/setcat")
                    .body(Body::from("2 1 Author 2 General"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        let reimu = tagged_trie.find("灵梦").unwrap();
        assert_eq!(reimu[0].1.category, Category::Author);

        let marisa = tagged_trie.find("魔理沙").unwrap();
        assert_eq!(marisa[0].1.category, Category::General);
    }

    #[tokio::test]
    async fn test_delete_tag() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/deltag")
                    .body(Body::from("1"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        let reimu = tagged_trie.find("灵梦").unwrap();
        assert_eq!(reimu.len(), 0);

        let marisa = tagged_trie.find("魔理沙").unwrap();
        assert_eq!(marisa.len(), 1);
    }

    #[tokio::test]
    async fn test_delete_word() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/delword")
                    .body(Body::from("博丽灵梦"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let tagged_trie = context.tagged_trie.read();
        let reimu1 = tagged_trie.find("灵梦").unwrap();
        assert_eq!(reimu1.len(), 0);

        let reimu2 = tagged_trie.find("红白").unwrap();
        assert_eq!(reimu2.len(), 1);
    }

    #[tokio::test]
    async fn test_match_first() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::POST)
                    .uri("/matchfirst")
                    .body(Body::from("2 博丽 东"))
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let body = hyper::body::to_bytes(response.into_body()).await.unwrap();
        let body: Value = serde_json::from_slice(&body).unwrap();

        assert_eq!(body, json!([1, 4]))
    }

    #[tokio::test]
    async fn test_root() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let uri = pathetic::Uri::default()
            .with_query_pairs_mut(|q| {
                q.append_pair("n", "10")
                    .append_pair("q", "东")
                    .append_pair("l", "CHS")
            })
            .to_string();

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::GET)
                    .uri(uri)
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let body = hyper::body::to_bytes(response.into_body()).await.unwrap();
        let body: Value = serde_json::from_slice(&body).unwrap();

        assert_eq!(
            body,
            json!([{
                "tag": "东方",
                "cat": 2,
                "cnt": 4
            },
            {
                "tag": "东风谷早苗",
                "cat": 1,
                "cnt": 3
            }])
        )
    }

    #[tokio::test]
    async fn test_ql() {
        let context = setup_context();
        let app = gen_router(context.clone());

        let uri = pathetic::Uri::default()
            .with_path("/ql")
            .with_query_pairs_mut(|q| q.append_pair("n", "10").append_pair("q", "东"))
            .to_string();

        let response = app
            .oneshot(
                Request::builder()
                    .method(http::Method::GET)
                    .uri(uri)
                    .body(Body::empty())
                    .unwrap(),
            )
            .await
            .unwrap();

        assert_eq!(response.status(), StatusCode::OK);

        let body = hyper::body::to_bytes(response.into_body()).await.unwrap();
        let ql_resps: Vec<QlResponse> = serde_json::from_slice(&body).unwrap();

        let ql_resps = ql_resps
            .into_iter()
            .map(|mut ql| {
                ql.langs.sort_by_key(|lang| lang.l);
                ql
            })
            .collect_vec();

        assert_eq!(
            serde_json::to_value(ql_resps).unwrap(),
            json!([{
                "cat": 2,
                "cnt": 4,
                "keyword": "东方",
                "langs": [
                    {"l": 1, "w": "东方"},
                    {"l": 5, "w": "Touhou"},
                ],
                "alias": [
                    "车万"
                ]
            },
            {
                "cat": 1,
                "cnt": 3,
                "keyword": "东风谷早苗",
                "langs": [
                    {
                        "l": 1,
                        "w": "东风谷早苗"
                    }
                ],
                "alias": []
            }])
        )
    }
}
