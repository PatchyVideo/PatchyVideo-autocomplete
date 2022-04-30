init:
	# do nothing

start:
	@cargo run

dev:
	@AUTOCOMPLETE_SERVER_CONFIG=./fixtures/dev.toml cargo run

test:
	@cargo test -- --nocapture