FROM scratch

# These commands copy your files into the specified directory in the image
# and set that as the working location
COPY autocomplete.app /autocomplete/
WORKDIR /autocomplete

# This command runs your application, comment out this line to compile only
CMD ["./autocomplete.app"]

LABEL Name=patchyvideo-autocomplete Version=0.0.1
