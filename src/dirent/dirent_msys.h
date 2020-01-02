


#ifndef DIRENT_DIRENT_MSYS_H
#define DIRENT_DIRENT_MSYS_H

#include <crtdefs.h>
#include <io.h>
#include <sys/stat.h>
typedef struct DIR DIR;

struct dirent
{
    long        d_ino;      /* Always zero. */
    unsigned short  d_reclen;   /* Always zero. */
    unsigned short  d_namlen;   /* Length of name in d_name. */
    char        d_name[260]; /* [FILENAME_MAX] */ /* File name. */
};
typedef struct dirent dirent;

struct _wdirent
{
    long        d_ino;      /* Always zero. */
    unsigned short  d_reclen;   /* Always zero. */
    unsigned short  d_namlen;   /* Length of name in d_name. */
    wchar_t     d_name[260]; /* [FILENAME_MAX] */ /* File name. */
};

typedef struct
{
    /* disk transfer area for this dir */
    struct _wfinddata_t dd_dta;

    /* dirent struct to return from dir (NOTE: this makes this thread
     * safe as long as only one thread uses a particular DIR struct at
     * a time) */
    struct _wdirent     dd_dir;

    /* _findnext handle */
    intptr_t        dd_handle;

    /*
     * Status of search:
     *   0 = not started yet (next entry to read is first entry)
     *  -1 = off the end
     *   positive = 0 based index of next entry
     */
    int         dd_stat;

    /* given path for dir with search pattern (struct is extended) */
    wchar_t         dd_name[1];
} _WDIR;


DIR *opendir_impl(const char *dirname);
dirent *readdir_impl (DIR *dirp);
int closedir_impl(DIR *dirp);

#define opendir opendir_impl
#define readdir readdir_impl
#define closedir closedir_impl

//int stat_utf8(const char *path, struct _stat64i32 *buffer);
#define stat64_t _stat64
int stat64_utf8(const char *path, struct stat64_t *buffer);
#endif