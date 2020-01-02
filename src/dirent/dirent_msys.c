

#include "dirent/dirent_msys.h"
#include <windows.h>

_WDIR* __cdecl  _wopendir (const wchar_t*);
struct _wdirent* __cdecl  _wreaddir (_WDIR*);
int __cdecl  _wclosedir (_WDIR*);

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

struct MSYS_DIR {
    struct dirent ent;
    _WDIR *wdirp;
};
typedef struct MSYS_DIR MSYS_DIR;

DIR *opendir_impl(const char *dirname)
{
    //return _opendir(dirname);
    MSYS_DIR* d2 = malloc(sizeof(struct MSYS_DIR));
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

dirent *readdir_impl (DIR *dirp)
{
    // return readdir(dirp);

    MSYS_DIR* d2 = (MSYS_DIR*)dirp;
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
    d2->ent.d_reclen = went->d_reclen;
    d2->ent.d_ino = went->d_ino;
    return &d2->ent;
}

int closedir_impl(DIR *dirp)
{
    //return _closedir(dirp);

    MSYS_DIR* d2 = (MSYS_DIR*)dirp;

    int v = _wclosedir(d2->wdirp);

    free(d2);
    return v;
}

/*
int stat_utf8(const char *path, struct _stat64i32 *buffer)
{
    wchar_t* wpath = utf8_2_utf16(path);
    int r = _wstat(wpath, buffer);
    free(wpath);
    return r;
}
*/


int stat64_utf8(const char *path, struct stat64_t *buffer)
{
    wchar_t* wpath = utf8_2_utf16(path);
    int r = _wstat64(wpath, buffer);
    free(wpath);
    return r;
}
