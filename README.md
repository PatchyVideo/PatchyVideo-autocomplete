# PatchyVideo-autocomplete
autocomplete part of PatchyVideo
# Usage
```C++
/*
*   POST /addtag         n tagid count cat ...    return ""
*   POST /addword        n tagid word lang ...    return ""
*   POST /setcount       n tagid count ...        return ""
*   POST /setcountdiff   n tagid diff ...         return ""
*   POST /setcat         n tagid cat ...          return ""
*   POST /deltag         tagid                    return ""
*   POST /delword        word                     return ""
*   POST /matchfirst     n words ...              return JSON[tagid,...]
*   GET  /?q=<prefix>&n=<max_words>&l=<lang>      return JSON[{word,category,count},...]
*   GET  /ql?q=<prefix>&n=<max_words>             return JSON[{category,count,matched keyword,langs:[{language,word},...],alias:[word,...]},...]
*/
```
# Compile
1. [Download](https://github.com/gcc-mirror/gcc) and compile latest gcc 12
2. ```./build.sh```
