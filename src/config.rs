use std::collections::{HashMap, HashSet};

use crate::{schema::Languages, tagged_trie::TaggedTrie};
use parking_lot::RwLock;
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize, PartialEq)]
pub struct Config {
    pub general: GeneralConfig,
    pub log: LogConfig,
    pub language_preference: HashMap<String, HashSet<Languages>>, // Enum as key are not supported yet
    pub mongodb: MongoDB,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
pub struct MongoDB {
    pub uri: String,
    pub database: String,
    pub collection: String,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
pub struct GeneralConfig {
    pub addr: String,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
pub struct LogConfig {
    pub path: String,
    pub rotation: RotationConfig,
    pub log_level: String,
    pub enable_log_file: bool,
}

#[derive(Debug, Serialize, Deserialize, PartialEq)]
pub enum RotationConfig {
    Hourly,
    Daily,
    Never,
}

#[derive(Debug)]
pub struct AppContext {
    pub config: Config,
    pub tagged_trie: RwLock<TaggedTrie>,
}
