/usr/local/bin/g++ -Ofast -s -flto -static -std=c++20 -o autocomplete.app autocomplete.cpp
docker build --no-cache -t patchyvideo-autocomplete:latest .
