
# PatchyVideo-autocomplete

Autocomplete part of PatchyVideo

## API Reference

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

## Deployment

To local run this project run

```bash
  make dev
```
