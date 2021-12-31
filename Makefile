init:
	# do nothing

start:
	@cargo run

dev:
	@AUTOCOMPLETE_SERVER_CONFIG=./fixtures/test.toml cargo run

test:
	@cargo test -- --nocapture