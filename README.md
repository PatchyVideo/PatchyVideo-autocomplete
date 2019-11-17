# PatchyVideo-autocomplete
autocomplete part of PatchyVideo
# Usage
```C++
/*
*   POST /addwords    n word cat num ...  return "" // must be called before POST /bulkalias
*   POST /addalias    n src dst ...       return ""
*   POST /setword     word freq           return "" // works on both word/alias
*   POST /delword     word                return "" // works on both word/alias
*   POST /delalias    src                 return "" // remove alias link, not deleting
*   GET  /?q=<prefix>&n=<max_words>       return JSON[{src,dst,category,freq},...]
*/
  ```
# Compile
1. [Download](https://github.com/gcc-mirror/gcc) and compile latest gcc(version 10.0.0)
2. ```g++ -fconcepts -O3 -std=c++2a -o autocomplete autocomplete.cpp```
