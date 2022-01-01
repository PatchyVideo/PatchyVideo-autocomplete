#!/bin/bash
NAME=patchyvideo-autocomplete
export PKG_CONFIG_ALLOW_CROSS=1
export OPENSSL_STATIC=true
export OPENSSL_DIR=/musl
cargo build --target x86_64-unknown-linux-musl --release
docker build --no-cache -t ${NAME} .
docker save -o ${NAME}.tar ${NAME}
