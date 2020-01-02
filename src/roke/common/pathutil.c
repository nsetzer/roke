

#include "roke/common/strutil.h"
#include "roke/common/pathutil.h"

/*
 * @brief build an absolute path from src.
 * use root as the current working directory for relative paths
 * remove any . or .. components from the path
 * this is purely a path computation, and does not check if the path exists
 * output will never terminate in a path separator, except for the root
 * directory on linux file systems.
 * @returns the length of the string on success
 *           on error, returns a value greater than ROKE_PATH_MAX
 *           either the output buffer is too small, or the file path is too long.
*/
size_t _abspath_impl(const uint8_t* src, size_t srclen,
                     uint8_t * dst, size_t dstlen,
                     const uint8_t* root, size_t rootlen)
{
    size_t i=0, j=0;

    if (memcmp("~/", src, 2)==0
        #ifdef _WIN32
        || memcmp("~\\", src, 2)==0
        #endif
        ) {
        // todo check for errors
        uint8_t* home = NULL;
        #ifdef _MSC_VER
            size_t homelen = 0;
            _dupenv_s(&home, &homelen, "USERPROFILE");

        #elif _WIN32
            home = (uint8_t*) getenv("USERPROFILE");
        #else
            home = (uint8_t*) getenv("HOME");
        #endif
        j = strcpy_safe(dst, dstlen, home);
        dst[j++] = SEP;
        i+=2;
    #ifdef _WIN32
    } else if (memcmp(":\\", src+1, 2)==0 ||
               memcmp(":/", src+1, 2)==0) {
        ;
    #else
    } else if (src[0] == SEP) {
        ;
    #endif
    } else {
        // todo check for errors
        j = strcpy_safe(dst, dstlen, root);
        dst[j++] = SEP;
        dst[j] = '\0';
    }

    while (src[i] != '\0' && i < srclen && j < (dstlen-1)) {
        if (memcmp(".." DSEP, src+i, 3)==0
            || memcmp("..\0", src+i, 3)==0
        #ifdef _WIN32
            || memcmp("../", src+i, 3)==0
        #endif
            ) {

            j-=2;
            while (j>0 && (dst[j] != SEP
            #ifdef _WIN32
                && dst[j] != '/'
            #endif
                )) {
                j--;
            }
            j++;
            i+=3;
        } else if (memcmp("." DSEP, src+i, 2)==0
            || memcmp(".", src+i, 2)==0
        #ifdef _WIN32
            || memcmp("./", src+i, 2)==0
        #endif
            ) {
            i += 2;
        } else if (dst[j-1] == SEP && src[i] == SEP) {
            i += 1;
        #ifdef _WIN32
        } else if (src[i] == '/') {
            dst[j++] = SEP;
            i++;
        #endif
        } else {
            dst[j++] = src[i++];
        }
    }

    #ifdef _WIN32
        if (j > 2 && dst[j-1] == SEP) {
            j--;
        }
    #else
        if (j > 1 && dst[j-1] == SEP) {
            j--;
        }
    #endif

    dst[j] = 0;

    // this implementation has an unintended side effect,
    // the success return value (j) can be greater than the error
    // return value (ROKE_PATH_MAX + 1) if the dest buffer has sufficient
    // length.
    if (i < srclen) {
        dst[0] = '\0';
        return ROKE_PATH_MAX + 1;
    }
    return j;
}

size_t _abspath(const uint8_t* src, size_t srclen, uint8_t* dst, size_t dstlen) {
    uint8_t root[ROKE_PATH_MAX];
    if (roke_getcwd((char*) root, sizeof(root))==NULL) {
        return -1;
    }
    return _abspath_impl(src, srclen, dst, dstlen, root, strlen((char*)root));
}

/**
 * @brief join path components into a single path
 * this is purely a path computation, and does not check if the path exists
 * output will never terminate in a path separator, except for the root
 * directory on linux file systems.
 *
 * @returns string length
 */
size_t _joinpath(const uint8_t** parts, size_t partslen, uint8_t* dst, size_t dstlen)
{

    size_t i=0, j=0;
    for (j=0; j < partslen; j++) {

        const uint8_t* tmp = parts[j];

        while (*tmp != '\0' && i < dstlen-1) {
            dst[i++] = *tmp++;
        }

        while (i>1 && (dst[i-1] == SEP
        #ifdef _WIN32
            || dst[i-1] == '/'
        #endif
            )) {
            i--;
        }

        if (j < partslen-1 && (i>0 && dst[i-1] != SEP))
            dst[i++] = SEP;
    }

    dst[i] = '\0';

    return i;
}

uint64_t creation_time(uint8_t *path)
{
    struct stat attr;
    stat((char*)path, &attr);
    return attr.st_mtime;
}

#ifdef _WIN32
    #if defined(_MSC_VER)
        #define mkdir_safe(x) _mkdir(x)
    #else
        #define mkdir_safe(x) mkdir(x)
    #endif
#else
    #define mkdir_safe(x) mkdir(x, S_IRWXU)
#endif

/**
 * @brief create directory and all intermediate directories
 * @param dir The full path to a directory to create.
 * @todo return error if dir cannot be created
 *
 * This function is equivalent to  'mkdir -p dir'
 */
int
makedirs(const uint8_t *dir) {
    int err=0;
    uint8_t tmp[ROKE_PATH_MAX];
    uint8_t *p = NULL;
    size_t len;
    snprintf((char*)tmp, sizeof(tmp), "%s", dir);
    len = strlen((char*)tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir_safe((char*)tmp);
            if(err!=0 && err!=EEXIST) {
                printf("*** mkdir fail 1 %s\n",dir);
                return err;
            }
            *p = '/';
        }
    }
    mkdir_safe((char*)tmp);

    if(err!=0 && err!=EEXIST) {
        printf("*** mkdir fail 2 %s\n",dir);
        return err;
    }

    return 0;
}


#if defined(_MSC_VER)
#include <share.h>
#endif

FILE* fopen_safe(const uint8_t * path, const char * mode)
{
#if defined(_MSC_VER)
    //errno_t err;
    FILE * f = NULL;
    errno = 0;
    // fopen_s has errno=13 PErmission Denied issues when used from python
    // could something not be correctly closing a handle?
    //err = fopen_s(&f, path, mode);
    f = _fsopen((char*)path, mode, _SH_DENYNO);
    if (errno != 0) {
        //printf(" *** Error opening file %s (errno=%d %s)\n", path,errno,strerror(errno));
        return NULL;
    }
    return f;
#else
    return fopen((char*)path, mode);
#endif
}