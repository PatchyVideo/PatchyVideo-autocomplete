# PatchyVideo-autocomplete
autocomplete part of PatchyVideo
# Usage
`
/*
*   POST /addwords    n word cat num ...  return "" // must be called before POST /bulkalias
*   POST /addalias    n src dst ...       return ""
*   POST /setword     word freq           return "" // works on both word/alias
*   POST /delword     word                return "" // works on both word/alias
*   POST /delalias    src                 return "" // remove alias link, not deleting
*   GET  /?q=<prefix>&n=<max_words>       return JSON[{src,dst,freq},...]
*/
  `
