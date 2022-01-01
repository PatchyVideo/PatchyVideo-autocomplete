FROM scratch

# These commands copy your files into the specified directory in the image
# and set that as the working location
COPY target/x86_64-unknown-linux-musl/release/patchy_video_autocomplete_server /webapp/app
WORKDIR /webapp

EXPOSE 5002
COPY ./fixtures/prod.toml .
ENV AUTOCOMPLETE_SERVER_CONFIG ./prod.toml

# This command runs your application, comment out this line to compile only
CMD ["./app"]
