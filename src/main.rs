mod config;
mod db;
mod handler;
mod route;
mod schema;
mod tagged_trie;

use anyhow::Result;
use config::{AppContext, Config, RotationConfig};
use db::fetch_all_tags_from_db;
use parking_lot::RwLock;
use route::gen_router;
use std::{env, str::FromStr, sync::Arc};
use tagged_trie::TaggedTrie;
use tokio::fs;
use tracing::{info, span};
use tracing_subscriber::{
    filter,
    fmt::{self, format},
    layer::SubscriberExt,
    prelude::*,
    EnvFilter,
};

#[tokio::main]
async fn main() -> Result<()> {
    // Creating the log

    let config = match env::var("AUTOCOMPLETE_SERVER_CONFIG") {
        Ok(path) => fs::read_to_string(&path)
            .await
            .expect("Cannot find the config file!"),
        Err(_) => include_str!("../fixtures/dev.toml").to_string(),
    };

    let config: Config = toml::from_str(&config).expect("Config must be a valid toml file");

    let log = &config.log;

    env::set_var("RUST_LOG", &log.log_level);

    let file_appender = match &log.rotation {
        RotationConfig::Hourly => tracing_appender::rolling::hourly(&log.path, "autocomplete.log"),
        RotationConfig::Daily => tracing_appender::rolling::daily(&log.path, "autocomplete.log"),
        RotationConfig::Never => tracing_appender::rolling::never(&log.path, "autocomplete.log"),
    };

    let (non_blocking, _guard) = tracing_appender::non_blocking(file_appender);

    let fmt_layer = fmt::layer()
        .event_format(format().compact())
        .with_writer(non_blocking);

    let level = filter::LevelFilter::from_str(&log.log_level)?;

    let log_file_level = match &log.enable_log_file {
        true => level,
        false => filter::LevelFilter::OFF,
    };

    let stdout_log = fmt::layer().compact();

    tracing_subscriber::registry()
        .with(EnvFilter::from_default_env())
        .with(stdout_log)
        .with(fmt_layer.with_filter(log_file_level))
        .init();

    let root = span!(tracing::Level::INFO, "autocomplete service start");
    let _enter = root.enter();

    info!("Getting the tags from mongodb");

    let tags = fetch_all_tags_from_db(&config)
        .await
        .expect("Cannot connect or fetch tags from mongodb");

    info!("Building trie from the tags");

    let tagged_trie = TaggedTrie::build_with_tags(&tags);

    let addr = config.general.addr.to_owned();

    let context = Arc::new(AppContext {
        config,
        tagged_trie: RwLock::new(tagged_trie),
    });

    let app = gen_router(context);

    info!("Starting the server");

    axum::Server::bind(&addr.parse().expect("Ip address must be valid"))
        .serve(app.into_make_service())
        .await?;

    Ok(())
}
