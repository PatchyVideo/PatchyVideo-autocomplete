FROM rust:latest AS chef
RUN cargo install cargo-chef

WORKDIR /app

FROM chef AS planner
COPY . .
RUN cargo chef prepare --recipe-path recipe.json

FROM chef AS builder
COPY --from=planner /app/recipe.json recipe.json
# Build dependencies - this is the caching Docker layer!
RUN cargo chef cook --release --recipe-path recipe.json

# Build application
COPY . .
RUN cargo install --path .

# We do not need the Rust toolchain to run the binary!
FROM gcr.io/distroless/cc
COPY --from=builder /usr/local/cargo/bin/patchy_video_autocomplete_server .
EXPOSE 5002
COPY ./fixtures/prod.toml .
ENV AUTOCOMPLETE_SERVER_CONFIG ./prod.toml
CMD [ "./patchy_video_autocomplete_server" ]