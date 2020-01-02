
#include "dirent/dirent.h"



/* Internal utility functions */
WIN32_FIND_DATAW *dirent_first (_WDIR *dirp);
WIN32_FIND_DATAW *dirent_next (_WDIR *dirp);

int dirent_mbstowcs_s(
    size_t *pReturnValue,
    wchar_t *wcstr,
    size_t sizeInWords,
    const char *mbstr,
    size_t count);

int dirent_wcstombs_s(
    size_t *pReturnValue,
    char *mbstr,
    size_t sizeInBytes,
    const wchar_t *wcstr,
    size_t count);

void dirent_set_errno (int error);


/*
 * Open directory stream DIRNAME for read and return a pointer to the
 * internal working area that is used to retrieve individual directory
 * entries.
 */
_WDIR*
_wopendir(
    const wchar_t *dirname)
{
    _WDIR *dirp = NULL;
    int error;

    /* Must have directory name */
    if (dirname == NULL  ||  dirname[0] == '\0') {
        dirent_set_errno (ENOENT);
        return NULL;
    }

    /* Allocate new _WDIR structure */
    dirp = (_WDIR*) malloc (sizeof (struct _WDIR));
    if (dirp != NULL) {
        DWORD n;

        /* Reset _WDIR structure */
        dirp->handle = INVALID_HANDLE_VALUE;
        dirp->patt = NULL;
        dirp->cached = 0;

        /* Compute the length of full path plus zero terminator
         *
         * Note that on WinRT there's no way to convert relative paths
         * into absolute paths, so just assume it is an absolute path.
         */
#       if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
            n = wcslen(dirname);
#       else
            n = GetFullPathNameW (dirname, 0, NULL, NULL);
#       endif

        /* Allocate room for absolute directory name and search pattern */
        dirp->patt = (wchar_t*) malloc (sizeof (wchar_t) * n + 16);
        if (dirp->patt) {

            /*
             * Convert relative directory name to an absolute one.  This
             * allows rewinddir() to function correctly even when current
             * working directory is changed between opendir() and rewinddir().
             *
             * Note that on WinRT there's no way to convert relative paths
             * into absolute paths, so just assume it is an absolute path.
             */
#           if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
                wcsncpy_s(dirp->patt, n+1, dirname, n);
#           else
                n = GetFullPathNameW (dirname, n, dirp->patt, NULL);
#           endif
            if (n > 0) {
                wchar_t *p;

                /* Append search pattern \* to the directory name */
                p = dirp->patt + n;
                if (dirp->patt < p) {
                    switch (p[-1]) {
                    case '\\':
                    case '/':
                    case ':':
                        /* Directory ends in path separator, e.g. c:\temp\ */
                        /*NOP*/;
                        break;

                    default:
                        /* Directory name doesn't end in path separator */
                        *p++ = '\\';
                    }
                }
                *p++ = '*';
                *p = '\0';

                /* Open directory stream and retrieve the first entry */
                if (dirent_first (dirp)) {
                    /* Directory stream opened successfully */
                    error = 0;
                } else {
                    /* Cannot retrieve first entry */
                    error = 1;
                    dirent_set_errno (ENOENT);
                }

            } else {
                /* Cannot retrieve full path name */
                dirent_set_errno (ENOENT);
                error = 1;
            }

        } else {
            /* Cannot allocate memory for search pattern */
            error = 1;
        }

    } else {
        /* Cannot allocate _WDIR structure */
        error = 1;
    }

    /* Clean up in case of error */
    if (error  &&  dirp) {
        _wclosedir (dirp);
        dirp = NULL;
    }

    return dirp;
}

/*
 * Read next directory entry.
 *
 * Returns pointer to directory entry which may be overwritten by
 * subsequent calls to _wreaddir().
 */
struct _wdirent*
_wreaddir(
    _WDIR *dirp)
{
    struct _wdirent *entry;

    /*
     * Read directory entry to buffer.  We can safely ignore the return value
     * as entry will be set to NULL in case of error.
     */
    (void) _wreaddir_r (dirp, &dirp->ent, &entry);

    /* Return pointer to statically allocated directory entry */
    return entry;
}

/*
 * Read next directory entry.
 *
 * Returns zero on success.  If end of directory stream is reached, then sets
 * result to NULL and returns zero.
 */
int
_wreaddir_r(
    _WDIR *dirp,
    struct _wdirent *entry,
    struct _wdirent **result)
{
    WIN32_FIND_DATAW *datap;

    /* Read next directory entry */
    datap = dirent_next (dirp);
    if (datap) {
        size_t n;
        DWORD attr;

        /*
         * Copy file name as wide-character string.  If the file name is too
         * long to fit in to the destination buffer, then truncate file name
         * to PATH_MAX characters and zero-terminate the buffer.
         */
        n = 0;
        while (n < PATH_MAX  &&  datap->cFileName[n] != 0) {
            entry->d_name[n] = datap->cFileName[n];
            n++;
        }
        entry->d_name[n] = 0;

        /* Length of file name excluding zero terminator */
        entry->d_namlen = n;

        /* File type */
        attr = datap->dwFileAttributes;
        if ((attr & FILE_ATTRIBUTE_DEVICE) != 0) {
            entry->d_type = DT_CHR;
        } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            entry->d_type = DT_DIR;
        } else {
            entry->d_type = DT_REG;
        }

        /* Reset dummy fields */
        entry->d_ino = 0;
        entry->d_off = 0;
        entry->d_reclen = sizeof (struct _wdirent);

        /* Set result address */
        *result = entry;

    } else {

        /* Return NULL to indicate end of directory */
        *result = NULL;

    }

    return /*OK*/0;
}

/*
 * Close directory stream opened by opendir() function.  This invalidates the
 * DIR structure as well as any directory entry read previously by
 * _wreaddir().
 */
int
_wclosedir(
    _WDIR *dirp)
{
    int ok;
    if (dirp) {

        /* Release search handle */
        if (dirp->handle != INVALID_HANDLE_VALUE) {
            FindClose (dirp->handle);
            dirp->handle = INVALID_HANDLE_VALUE;
        }

        /* Release search pattern */
        if (dirp->patt) {
            free (dirp->patt);
            dirp->patt = NULL;
        }

        /* Release directory structure */
        free (dirp);
        ok = /*success*/0;

    } else {

        /* Invalid directory stream */
        dirent_set_errno (EBADF);
        ok = /*failure*/-1;

    }
    return ok;
}

/*
 * Rewind directory stream such that _wreaddir() returns the very first
 * file name again.
 */
void
_wrewinddir(
    _WDIR* dirp)
{
    if (dirp) {
        /* Release existing search handle */
        if (dirp->handle != INVALID_HANDLE_VALUE) {
            FindClose (dirp->handle);
        }

        /* Open new search handle */
        dirent_first (dirp);
    }
}

/* Get first directory entry (internal) */
WIN32_FIND_DATAW*
dirent_first(
    _WDIR *dirp)
{
    WIN32_FIND_DATAW *datap;

    /* Open directory and retrieve the first entry */
    dirp->handle = FindFirstFileExW(
        dirp->patt, FindExInfoStandard, &dirp->data,
        FindExSearchNameMatch, NULL, 0);
    if (dirp->handle != INVALID_HANDLE_VALUE) {

        /* a directory entry is now waiting in memory */
        datap = &dirp->data;
        dirp->cached = 1;

    } else {

        /* Failed to re-open directory: no directory entry in memory */
        dirp->cached = 0;
        datap = NULL;

    }
    return datap;
}

/*
 * Get next directory entry (internal).
 *
 * Returns
 */
WIN32_FIND_DATAW*
dirent_next(
    _WDIR *dirp)
{
    WIN32_FIND_DATAW *p;

    /* Get next directory entry */
    if (dirp->cached != 0) {

        /* A valid directory entry already in memory */
        p = &dirp->data;
        dirp->cached = 0;

    } else if (dirp->handle != INVALID_HANDLE_VALUE) {

        /* Get the next directory entry from stream */
        if (FindNextFileW (dirp->handle, &dirp->data) != FALSE) {
            /* Got a file */
            p = &dirp->data;
        } else {
            /* The very last entry has been processed or an error occurred */
            FindClose (dirp->handle);
            dirp->handle = INVALID_HANDLE_VALUE;
            p = NULL;
        }

    } else {

        /* End of directory stream reached */
        p = NULL;

    }

    return p;
}

/*
 * Open directory stream using plain old C-string.
 */
DIR*
_opendir(
    const char *dirname)
{
    struct DIR *dirp;
    int error;

    /* Must have directory name */
    if (dirname == NULL  ||  dirname[0] == '\0') {
        dirent_set_errno (ENOENT);
        return NULL;
    }

    /* Allocate memory for DIR structure */
    dirp = (DIR*) malloc (sizeof (struct DIR));
    if (dirp) {
        wchar_t wname[PATH_MAX + 1];
        size_t n;

        /* Convert directory name to wide-character string */
        error = dirent_mbstowcs_s(
            &n, wname, PATH_MAX + 1, dirname, PATH_MAX + 1);
        if (!error) {

            /* Open directory stream using wide-character name */
            dirp->wdirp = _wopendir (wname);
            if (dirp->wdirp) {
                /* Directory stream opened */
                error = 0;
            } else {
                /* Failed to open directory stream */
                error = 1;
            }

        } else {
            /*
             * Cannot convert file name to wide-character string.  This
             * occurs if the string contains invalid multi-byte sequences or
             * the output buffer is too small to contain the resulting
             * string.
             */
            error = 1;
        }

    } else {
        /* Cannot allocate DIR structure */
        error = 1;
    }

    /* Clean up in case of error */
    if (error  &&  dirp) {
        free (dirp);
        dirp = NULL;
    }

    return dirp;
}

/*
 * Read next directory entry.
 */
struct dirent*
_readdir(
    DIR *dirp)
{
    struct dirent *entry;

    /*
     * Read directory entry to buffer.  We can safely ignore the return value
     * as entry will be set to NULL in case of error.
     */
    (void) readdir_r (dirp, &dirp->ent, &entry);

    /* Return pointer to statically allocated directory entry */
    return entry;
}

/*
 * Read next directory entry into called-allocated buffer.
 *
 * Returns zero on success.  If the end of directory stream is reached, then
 * sets result to NULL and returns zero.
 */
int
readdir_r(
    DIR *dirp,
    struct dirent *entry,
    struct dirent **result)
{
    WIN32_FIND_DATAW *datap;

    /* Read next directory entry */
    datap = dirent_next (dirp->wdirp);
    if (datap) {
        size_t n;
        int error;

        /* Attempt to convert file name to multi-byte string */
        error = dirent_wcstombs_s(
            &n, entry->d_name, PATH_MAX + 1, datap->cFileName, PATH_MAX + 1);

        /*
         * If the file name cannot be represented by a multi-byte string,
         * then attempt to use old 8+3 file name.  This allows traditional
         * Unix-code to access some file names despite of unicode
         * characters, although file names may seem unfamiliar to the user.
         *
         * Be ware that the code below cannot come up with a short file
         * name unless the file system provides one.  At least
         * VirtualBox shared folders fail to do this.
         */
        if (error  &&  datap->cAlternateFileName[0] != '\0') {
            error = dirent_wcstombs_s(
                &n, entry->d_name, PATH_MAX + 1,
                datap->cAlternateFileName, PATH_MAX + 1);
        }

        if (!error) {
            DWORD attr;

            /* Length of file name excluding zero terminator */
            entry->d_namlen = n - 1;

            /* File attributes */
            attr = datap->dwFileAttributes;
            if ((attr & FILE_ATTRIBUTE_DEVICE) != 0) {
                entry->d_type = DT_CHR;
            } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                entry->d_type = DT_DIR;
            } else {
                entry->d_type = DT_REG;
            }

            /* Reset dummy fields */
            entry->d_ino = 0;
            entry->d_off = 0;
            entry->d_reclen = sizeof (struct dirent);

        } else {

            /*
             * Cannot convert file name to multi-byte string so construct
             * an erroneous directory entry and return that.  Note that
             * we cannot return NULL as that would stop the processing
             * of directory entries completely.
             */
            entry->d_name[0] = '?';
            entry->d_name[1] = '\0';
            entry->d_namlen = 1;
            entry->d_type = DT_UNKNOWN;
            entry->d_ino = 0;
            entry->d_off = -1;
            entry->d_reclen = 0;

        }

        /* Return pointer to directory entry */
        *result = entry;

    } else {

        /* No more directory entries */
        *result = NULL;

    }

    return /*OK*/0;
}

/*
 * Close directory stream.
 */
int
_closedir(
    DIR *dirp)
{
    int ok;
    if (dirp) {

        /* Close wide-character directory stream */
        ok = _wclosedir (dirp->wdirp);
        dirp->wdirp = NULL;

        /* Release multi-byte character version */
        free (dirp);

    } else {

        /* Invalid directory stream */
        dirent_set_errno (EBADF);
        ok = /*failure*/-1;

    }
    return ok;
}

/*
 * Rewind directory stream to beginning.
 */
void
rewinddir(
    DIR* dirp)
{
    /* Rewind wide-character string directory stream */
    _wrewinddir (dirp->wdirp);
}

/*
 * Scan directory for entries.
 */
int
scandir(
    const char *dirname,
    struct dirent ***namelist,
    int (*filter)(const struct dirent*),
    int (*compare)(const void*, const void*))
{
    struct dirent **files = NULL;
    size_t size = 0;
    size_t allocated = 0;
    const size_t init_size = 1;
    DIR *dir = NULL;
    struct dirent *entry;
    struct dirent *tmp = NULL;
    size_t i;
    int result = 0;

    /* Open directory stream */
    dir = _opendir (dirname);
    if (dir) {

        /* Read directory entries to memory */
        while (1) {

            /* Enlarge pointer table to make room for another pointer */
            if (size >= allocated) {
                void *p;
                size_t num_entries;

                /* Compute number of entries in the enlarged pointer table */
                if (size < init_size) {
                    /* Allocate initial pointer table */
                    num_entries = init_size;
                } else {
                    /* Double the size */
                    num_entries = size * 2;
                }

                /* Allocate first pointer table or enlarge existing table */
                p = realloc (files, sizeof (void*) * num_entries);
                if (p != NULL) {
                    /* Got the memory */
                    files = (dirent**) p;
                    allocated = num_entries;
                } else {
                    /* Out of memory */
                    result = -1;
                    break;
                }

            }

            /* Allocate room for temporary directory entry */
            if (tmp == NULL) {
                tmp = (struct dirent*) malloc (sizeof (struct dirent));
                if (tmp == NULL) {
                    /* Cannot allocate temporary directory entry */
                    result = -1;
                    break;
                }
            }

            /* Read directory entry to temporary area */
            if (readdir_r (dir, tmp, &entry) == /*OK*/0) {

                /* Did we get an entry? */
                if (entry != NULL) {
                    int pass;

                    /* Determine whether to include the entry in result */
                    if (filter) {
                        /* Let the filter function decide */
                        pass = filter (tmp);
                    } else {
                        /* No filter function, include everything */
                        pass = 1;
                    }

                    if (pass) {
                        /* Store the temporary entry to pointer table */
                        files[size++] = tmp;
                        tmp = NULL;

                        /* Keep up with the number of files */
                        result++;
                    }

                } else {

                    /*
                     * End of directory stream reached => sort entries and
                     * exit.
                     */
                    qsort (files, size, sizeof (void*), compare);
                    break;

                }

            } else {
                /* Error reading directory entry */
                result = /*Error*/ -1;
                break;
            }

        }

    } else {
        /* Cannot open directory */
        result = /*Error*/ -1;
    }

    /* Release temporary directory entry */
    if (tmp) {
        free (tmp);
    }

    /* Release allocated memory on error */
    if (result < 0) {
        for (i = 0; i < size; i++) {
            free (files[i]);
        }
        free (files);
        files = NULL;
    }

    /* Close directory stream */
    if (dir) {
        closedir (dir);
    }

    /* Pass pointer table to caller */
    if (namelist) {
        *namelist = files;
    }
    return result;
}

/* Alphabetical sorting */
int
alphasort(
    const struct dirent **a, const struct dirent **b)
{
    return strcoll ((*a)->d_name, (*b)->d_name);
}

/* Sort versions */
int
versionsort(
    const struct dirent **a, const struct dirent **b)
{
    /* FIXME: implement strverscmp and use that */
    return alphasort (a, b);
}


/* Convert multi-byte string to wide character string */
int
dirent_mbstowcs_s(
    size_t *pReturnValue,
    wchar_t *wcstr,
    size_t sizeInWords,
    const char *mbstr,
    size_t count)
{
    int error;

#if defined(_MSC_VER)  &&  _MSC_VER >= 1400

    /* Microsoft Visual Studio 2005 or later */
    error = mbstowcs_s (pReturnValue, wcstr, sizeInWords, mbstr, count);

#else

    /* Older Visual Studio or non-Microsoft compiler */
    size_t n;

    /* Convert to wide-character string (or count characters) */
    n = mbstowcs (wcstr, mbstr, sizeInWords);
    if (!wcstr  ||  n < count) {

        /* Zero-terminate output buffer */
        if (wcstr  &&  sizeInWords) {
            if (n >= sizeInWords) {
                n = sizeInWords - 1;
            }
            wcstr[n] = 0;
        }

        /* Length of resulting multi-byte string WITH zero terminator */
        if (pReturnValue) {
            *pReturnValue = n + 1;
        }

        /* Success */
        error = 0;

    } else {

        /* Could not convert string */
        error = 1;

    }

#endif

    return error;
}

/* Convert wide-character string to multi-byte string */
int
dirent_wcstombs_s(
    size_t *pReturnValue,
    char *mbstr,
    size_t sizeInBytes, /* max size of mbstr */
    const wchar_t *wcstr,
    size_t count)
{
    int error;

#if defined(_MSC_VER)  &&  _MSC_VER >= 1400

    /* Microsoft Visual Studio 2005 or later */
    error = wcstombs_s (pReturnValue, mbstr, sizeInBytes, wcstr, count);

#else

    /* Older Visual Studio or non-Microsoft compiler */
    size_t n;

    /* Convert to multi-byte string (or count the number of bytes needed) */
    n = wcstombs (mbstr, wcstr, sizeInBytes);
    if (!mbstr  ||  n < count) {

        /* Zero-terminate output buffer */
        if (mbstr  &&  sizeInBytes) {
            if (n >= sizeInBytes) {
                n = sizeInBytes - 1;
            }
            mbstr[n] = '\0';
        }

        /* Length of resulting multi-bytes string WITH zero-terminator */
        if (pReturnValue) {
            *pReturnValue = n + 1;
        }

        /* Success */
        error = 0;

    } else {

        /* Cannot convert string */
        error = 1;

    }

#endif

    return error;
}

/* Set errno variable */
void
dirent_set_errno(
    int error)
{
#if defined(_MSC_VER)  &&  _MSC_VER >= 1400

    /* Microsoft Visual Studio 2005 and later */
    _set_errno (error);

#else

    /* Non-Microsoft compiler or older Microsoft compiler */
    errno = error;

#endif
}

wchar_t* utf8_2_utf16(const char* u8str) {
    int size = (int) strlen(u8str);
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, u8str, size, NULL, 0);
    wchar_t* wstrTo = (wchar_t*) malloc(sizeof(wchar_t) * (size_needed+1));
    int t = MultiByteToWideChar(CP_UTF8, 0, u8str, size, wstrTo, size_needed);
    wstrTo[t] = '\0';
    return wstrTo;
}

char* utf16_2_utf8(const wchar_t* u16str) {
    int size = (int) wcslen(u16str);
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, u16str, size, NULL, 0, 0, 0);
    char* strTo = (char*) malloc(sizeof(char) * (size_needed+1));
    int t = WideCharToMultiByte(CP_UTF8, 0, u16str, size, strTo, size_needed, 0, 0);
    strTo[t] = '\0';
    return strTo;
}

/**
 * @brief A directory which transparently converts UTF8 to UTF16 and back
 *
 * this mixes the DIR and _WDIR definitions in this file, and automatically
 * handles the conversion from utf8 text to utf16 and back.
 * this allows for unicode support on windows file systems.
 */
struct WWDIR {
    struct dirent ent;
    struct _WDIR *wdirp;
};
typedef struct WWDIR WWDIR;

DIR *opendir(const char *dirname)
{
    //return _opendir(dirname);
    WWDIR* d2 = malloc(sizeof(struct WWDIR));
    wchar_t* path = utf8_2_utf16(dirname);
    d2->wdirp = _wopendir(path);
    if (!d2->wdirp) {
        free(path);
        free(d2);
        return NULL;
    }

    free(path);
    DIR* d = (DIR*) d2;
    return d;

}

dirent *readdir (DIR *dirp)
{
    // return readdir(dirp);

    WWDIR* d2 = (WWDIR*)dirp;
    struct _wdirent* went = NULL;
    went = _wreaddir(d2->wdirp);
    if (went == NULL) {
        return NULL;
    }
    // convert the wide-char name to utf8
    int t = WideCharToMultiByte(CP_UTF8, 0,
        went->d_name, (int) wcslen(went->d_name),
        d2->ent.d_name, MAX_PATH+1, 0, 0);
    d2->ent.d_name[t] = '\0';
    // copy the wide-char traits to the utf8 entry
    d2->ent.d_namlen = t;
    d2->ent.d_type = went->d_type;
    d2->ent.d_off = went->d_off;
    d2->ent.d_reclen = went->d_reclen;
    d2->ent.d_ino = went->d_ino;
    return &d2->ent;
}

int closedir(DIR *dirp)
{
    //return _closedir(dirp);

    WWDIR* d2 = (WWDIR*)dirp;

    int v = _wclosedir(d2->wdirp);

    free(d2);
    return v;
}

int stat_utf8(const char *path, struct _stat *buffer)
{
    wchar_t* wpath = utf8_2_utf16(path);
    int r = _wstat(wpath, buffer);
    free(wpath);
    return r;
}

int stat64_utf8(const char *path, struct stat64_t *buffer)
{
    wchar_t* wpath = utf8_2_utf16(path);
    int r = _wstat(wpath, buffer);
    free(wpath);
    return r;
}
