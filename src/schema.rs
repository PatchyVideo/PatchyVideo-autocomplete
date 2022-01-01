use std::collections::HashSet;

use num_enum::{IntoPrimitive, TryFromPrimitive};
use patchy_video_autocomplete_macros::{FromStrEnum, ParseRequest};
use serde::{Deserialize, Serialize};
use std::string::ToString;
use strum_macros::{Display, EnumIter};

#[derive(Debug, ParseRequest)]
pub struct AddTagRequest {
    pub tagid: u32,
    pub count: u32,
    pub category: Category,
}

#[derive(Debug, ParseRequest)]
pub struct AddWordRequest {
    pub tagid: u32,
    pub word: String,
    pub lang: Languages,
}

#[derive(Debug, ParseRequest)]
pub struct SetCountRequest {
    pub tagid: u32,
    pub count: u32,
}

#[derive(Debug, ParseRequest)]
pub struct SetCountDiffRequest {
    pub tagid: u32,
    pub diff: i32,
}

#[derive(Debug, ParseRequest)]
pub struct SetCatRequest {
    pub tagid: u32,
    pub cat: Category,
}

#[derive(Debug, ParseRequest)]
pub struct MatchFirstRequest {
    pub word: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct RootResponse {
    #[serde(rename(serialize = "tag"))]
    pub word: String,
    #[serde(rename(serialize = "cat"))]
    pub category: u32,
    #[serde(rename(serialize = "cnt"))]
    pub count: u32,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct QlResponseLanguages {
    pub l: u32,
    pub w: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct QlResponse {
    #[serde(rename = "cat")]
    pub category: u32,
    #[serde(rename = "cnt")]
    pub count: u32,
    #[serde(rename = "keyword")]
    pub matched_keyword: String,
    pub langs: Vec<QlResponseLanguages>,
    pub alias: HashSet<String>,
}

#[derive(Debug, FromStrEnum, Clone, Copy, Serialize, Deserialize, PartialEq)]
pub enum Category {
    General,
    Character,
    Copyright,
    Author,
    Meta,
    Language,
    Soundtrack,
}

#[derive(
    Debug,
    Clone,
    Copy,
    FromStrEnum,
    EnumIter,
    Display,
    Serialize,
    Deserialize,
    PartialEq,
    Eq,
    Hash,
    TryFromPrimitive,
    IntoPrimitive,
)]
#[repr(u32)]
pub enum Languages {
    NAL,
    CHS,
    CHT,
    CSY,
    NLD,
    ENG,
    FRA,
    DEU,
    HUN,
    ITA,
    JPN,
    KOR,
    PLK,
    PTB,
    ROM,
    RUS,
    ESP,
    TRK,
    VIN,
}
