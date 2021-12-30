use crate::{
    schema::{Category, Languages},
    Config,
};
use anyhow::Result;
use futures::stream::TryStreamExt;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct Tag {
    pub id: u32,
    pub category: Category,
    pub count: u32,
    pub icon: String,
    pub alias: HashSet<String>,
    pub languages: HashMap<Languages, String>,
}

pub async fn fetch_all_tags_from_db(config: &Config) -> Result<Vec<Tag>> {
    let client_uri = &config.mongodb.uri;
    let client = mongodb::Client::with_uri_str(client_uri).await?;

    let patchyvideo = client.database(&config.mongodb.database);

    let tags = patchyvideo
        .collection::<Tag>(&config.mongodb.collection)
        .find(None, None)
        .await?
        .try_collect::<Vec<Tag>>()
        .await?;

    Ok(tags)
}
