g++ -Ofast -s -static -std=c++2a -fcoroutines -o autocomplete.app autocomplete.cpp
docker build --no-cache -t patchyvideo-autocomplete:latest .
