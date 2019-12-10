# PatchyVideo-autocomplete
autocomplete part of PatchyVideo
# Usage
```C++
/*
*   POST /addtag         n tagid count cat ...  return ""
*   POST /addword        n tagid word ...       return ""
*   POST /setcount       n tagid count ...      return ""
*   POST /setcountdiff   n tagid diff ...       return ""
*   POST /deltag         tagid                  return ""
*   POST /delword        word                   return ""
*   GET  /?q=<prefix>&n=<max_words>             return JSON[{tag,cat,cnt},...]
*/
```
# Compile
1. [Download](https://github.com/gcc-mirror/gcc) and compile latest gcc(version 10.0.0)
2. ```./build.sh```
