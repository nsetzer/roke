
#include "roke/common/regex.h"
#include "roke/common/pathutil.h"
#include "roke/common/strutil.h"
#ifdef _WIN32
    #include "trex/trex.h"
#else
    #include <regex.h>
#endif

int regex_compile(rregex_t* prex, const uint8_t* pattern, size_t patlen)
{
#ifdef _WIN32
    //trex
    prex->error = NULL;
    prex->ptr = (void*) trex_compile((const char *) pattern, (const char**) &prex->error);
    return (prex->ptr == NULL) ? 1 : 0;
#else
    // gnu
    prex->ptr = malloc(sizeof(regex_t));
    return regcomp(prex->ptr, (const char *)pattern, REG_EXTENDED);
#endif

}

int regex_match(rregex_t* prex, const uint8_t* str, size_t strlen)
{
#ifdef _WIN32
    //trex
    return trex_match(prex->ptr, (char*) str) ? 0 : 1;
#else
    // gnu
    return regexec(prex->ptr, (char*) str, 0, NULL, 0);
#endif
}

int regex_free(rregex_t* prex)
{
#ifdef _WIN32
    //trex
    trex_free(prex->ptr);
#else
    // gnu
    regfree(prex->ptr);
    free(prex->ptr);
#endif
    return 0;
}