# PatchyVideo-autocomplete
autocomplete part of PatchyVideo
# Usage
```C++
/*
*   POST /addwords       n word cat freq ...  return "" // must be called before POST /addalias
*   POST /addalias       n src dst ...        return ""
*   POST /setwords       n word freq ...      return "" // works on both word/alias
*   POST /setwordsdiff   n word diff ...      return "" // works on both word/alias
*   POST /delword        word                 return "" // works on both word/alias
*   POST /delalias       src                  return "" // remove alias link, not deleting
*   GET  /?q=<prefix>&n=<max_words>           return JSON[{src,dst,category,freq},...]
*/
```
# Compile
1. [Download](https://github.com/gcc-mirror/gcc) and compile latest gcc(version 10.0.0)
2. ```./build.sh```
