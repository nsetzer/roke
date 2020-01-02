
#include "roke/common/strutil.h"
#include "roke/common/pathutil.h"
#include "utf8/utf8proc.h"

size_t tolowercase(uint8_t* dst, size_t dstlen, const uint8_t* str)
{
      utf8proc_ssize_t inplen = strlen((char*) str);
      utf8proc_int32_t codepoint;
      const utf8proc_uint8_t* inptmp = str;
      utf8proc_uint8_t* tmpout = dst;
      utf8proc_ssize_t count;
      utf8proc_ssize_t total=0;

      // reserve one byte for the null terminator
      dstlen -= 1;

      // decode the first code point in the string
      count = utf8proc_iterate(inptmp, inplen, &codepoint);

      // loop while characters remain, and we encounter no errors.
      while (inplen>0 && codepoint!=-1) {
        inplen -= count;
        inptmp += count;

        // convert the code point to lowercase if possible
        codepoint = utf8proc_tolower(codepoint);

        // check that the output contains enough space for the input
        if ((dstlen - total) < 4) {
            total = 0;
            break;
        }

        // reencode the character to a byte string
        count = utf8proc_encode_char(codepoint, tmpout);
        tmpout += count;
        total += count;

        // decode the next code point
        count = utf8proc_iterate(inptmp, inplen, &codepoint);
      }

      dst[total] = '\0';

      return (size_t) total;
}

uint8_t*
strdup_safe(const uint8_t *str)
{
    size_t n = strlen((char*)str) + 1;
    uint8_t *dup = malloc(n);
    if(dup)
    {
        strcpy_safe(dup, n, str);
    }
    return dup;
}

size_t
strcpy_safe(uint8_t *dst, size_t dst_len, const uint8_t *str)
{
    size_t i;
    for(i=0; i<dst_len && str[i]!='\0'; i++) {
        dst[i] = str[i];
    }
    // check for errors
    if (i==dst_len) {
        dst[0] = '\0';
        return -1;
    }
    dst[i] = '\0';
    return i;
}

int
has_suffix(const uint8_t *str, size_t lenstr, const uint8_t *suffix, size_t lensuffix)
{
    if (!str || !suffix)
        return 0;
    if (lensuffix >  lenstr)
        return 0;
    return strncmp((char*)(str + lenstr - lensuffix),
                   (char*)suffix, lensuffix) == 0;
}

/**
 * @brief simple unix-glob-like pattern matching
 * @param ptn The pattern string
 * @param str The string to match
 *
 * Match a string to a pattern using '.' and '*' as a wild card.
 *
 * @return zero if string matches pattern. non-zero if it does not match.
 */
int
strglob(const uint8_t *ptn, const uint8_t *str) {

    int32_t r = (int32_t) strlen((char*)ptn);
    int32_t l = (int32_t) strlen((char*)str);
    int32_t i = 0, j = 0;
    int32_t t;
    int m;

    // TODO: short circuit, if (ptn[0] = '*' && ptn[1] = '\0')

    while (i < r && j < l) {
        //printf(":: - %s[%d]/%d %s[%d]/%d\n", ptn, i, r, str, j, l);

        switch(ptn[i]) {
            case '\\':
                // check that this is not the last character in the ptn
                if (i+1 >= r) {
                    return 4;
                }
                // check that the next character matches
                if (ptn[i+1] != str[j]) {
                    return 3;
                }
                i+=2;
                j+=1;
                break;
            case '*':
                // greedy match, use recursion
                //printf(":: * %s[%d]/%d %s[%d]/%d\n", ptn, i, r, str, j, l);

                i++;
                if (ptn[i] =='\\')
                    i++;

                for (t = l-1; t >= j; t--) {
                    //printf(":: r %s[%d]/%d %s[%d]/%d\n", ptn, i, r, str, j, l);
                    if (i==r) {
                        // match all until end of string
                        return 0;
                    } else if (ptn[i]==str[t]) {
                        m = strglob(ptn+i, str+t);
                        if (m==0)
                            return 0;
                    }

                }
                return 2;
                break;
            case '?':
                i++;
                j++;
                break;
            // case '['
            //  [seq] - match any character in seq
            //  [!seq] - match any character not in seq
            default:
                if (ptn[i] != str[j])
                    return 1;
                i++;
                j++;
                break;
        }
    }
    //printf(":: . %s[%d] %s[%d]\n", ptn, i, str, j);
    return (i == r && j == l)?0:1; // pattern matched until end of string
}

int
strescape(uint8_t* str, size_t len)
{
    size_t i=0, j=0;
    for (;j<len;) {
        fflush(stdout);
        if (str[j] == '\\') {
            switch (str[j + 1]) {
                case 'n':
                    str[i++] = '\n';
                    j++;
                    j++;
                    break;

                case '\0':
                    str[i++] = str[j++];
                    break;
                case '\\':
                    // skip the second slash
                    str[i++] = str[j++];
                    j++;
                    break;

                case 'x':
                    fflush(stdout);

                    if (j+4 <= len) {

                        char a[3] = {str[j+2], str[j+3], 0};
                        str[i++] = (uint8_t) strtol(a, NULL, 16);
                        j += 4;
                    } else {
                        str[i++] = str[j++];
                    }
                    break;

                default:
                    str[i++] = str[j++];
            }
        } else {
            str[i++] = str[j++];
        }
    }
    str[i]='\0';
    return 0;
}