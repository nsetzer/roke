/*
use three index files
    directory index
    file index
    strings
write binary structures directly to each index file
when done, combine the files into 1, by catting, and
recording the byte offsets in the header.

the strings can be indexed by
    (offset from start strings file) + (location of strings file )
*/

#include "roke/libroke_internal.h"

#define ROKE_INDEX_SIZE 1

#ifndef ROKE_INDEX_SIZE
    #define ROKE_INDEX_SIZE 0
#endif

#ifdef _DIRENT_HAVE_D_TYPE
#else
#warning  "_DIRENT_HAVE_D_TYPE not defined. Indexing will be slow"

#endif

int
string_matcher_init(
    string_matcher_t* matcher,
    const uint8_t* pattern,
    size_t patlen,
    int flags)
{
    int err = 0;
    matcher->flags = flags;
    const uint8_t* tmp = NULL;

    matcher->scratch = malloc(sizeof(uint8_t) * ROKE_PATH_MAX);
    if (!matcher->scratch)
        return 1;

    switch (flags&ROKE_MATCH_MASK) {
        case ROKE_GLOB:
            tmp = pattern;
            if (flags&ROKE_CASE_INSENSITIVE) {
                patlen = tolowercase(matcher->scratch, ROKE_PATH_MAX, pattern);
                tmp = matcher->scratch;
            }

            matcher->data.glob_pattern = strdup_safe(tmp);
            break;
        case ROKE_REGEX:
            err = regex_compile(&matcher->data.regex, pattern, patlen);
            break;
        default:
            tmp = pattern;
            if (flags&ROKE_CASE_INSENSITIVE) {
                patlen = tolowercase(matcher->scratch, ROKE_PATH_MAX, pattern);
                tmp = matcher->scratch;
            }
            err = boyer_moore_init(&matcher->data.bmopt, tmp, patlen);
            break;

    }
    return err;
}

int
string_matcher_match(
    string_matcher_t* matcher,
    const uint8_t* str,
    size_t len)
{
    int err = 0;
    const uint8_t* s = str;
    if (matcher->flags&ROKE_CASE_INSENSITIVE) {
        len = tolowercase(matcher->scratch, ROKE_PATH_MAX, str);
        s = matcher->scratch;
    }

    switch ((matcher->flags)&ROKE_MATCH_MASK) {
        case ROKE_GLOB:
            err = strglob(matcher->data.glob_pattern, s);
            break;
        case ROKE_REGEX:
            err = regex_match(&matcher->data.regex, s, len);
            break;
        default:
            err = (boyer_moore_match(&matcher->data.bmopt,
                                     s, len)!=NULL)?0:1;
            break;

    }
    return err;
}

int
string_matcher_free(
    string_matcher_t* matcher)
{
    int err = 0;
    switch ((matcher->flags)&ROKE_MATCH_MASK) {
        case ROKE_GLOB:
            free(matcher->data.glob_pattern);
            break;
        case ROKE_REGEX:
            regex_free(&matcher->data.regex);
            break;
        default:
            boyer_moore_free(&matcher->data.bmopt);
            break;

    }
    return err;
}

size_t _nstatcalls=0;
int roke_dirent_info(
    struct dirent *dir,
    uint8_t* path,
    int* is_dir,
    off_t* size)
{
    *is_dir = 0;
    *size = 0;
    #ifdef _DIRENT_HAVE_D_TYPE
        if (dir->d_type != DT_UNKNOWN && dir->d_type != DT_LNK && (ROKE_INDEX_SIZE)==0) {
            // don't have to stat if we have d_type info, unless
            // it's a symlink (since we stat, not lstat)
            *is_dir = (dir->d_type == DT_DIR);
        } else
    #endif
        {
            // if d_type isn't available, fallback for file systems
            // where the kernel returns DT_UNKNOWN.
            _nstatcalls++;
            struct stat64_t stbuf;
            // stat follows symlinks, lstat doesn't.
            if (stat64_utf8((char*) path, &stbuf)==0) {
                *is_dir = S_ISDIR(stbuf.st_mode);
                *size = stbuf.st_size;
            } else {
                fprintf(stderr, "error: unable to stat: %s\n ", (char*)path);
                return -1;
            }
        }

    return 0;
}

/**
 * @brief get the default config directory
 * @param dst    the buffer to write too
 * @param dstlen the length of the buffer
 * @return       the length of the string. if length is longer than dstlen
 *               a memory error occured and the caller should try again with
 *               a buffer at least as many bytes as the return value suggests
 *
 * uses the users HOME environment variable to determine a default location
 * to store the roke configuration (index files)
 */
size_t
roke_default_config_dir(
    char* dst,
    size_t dstlen)
{
    #if defined(_MSC_VER)
        char* home = NULL;
        size_t homelen = 0;
        _dupenv_s(&home, &homelen, "USERPROFILE");
        const char * parts[] = { home, ".config" DSEP "roke" };
    #elif _WIN32
        const char * parts[] = {getenv("USERPROFILE"), ".config" DSEP "roke"};
    #else
        const char * parts[] = {getenv("HOME"), ".config" DSEP "roke"};
    #endif
    if (parts[0] == NULL) {
        return 0;
    }
    size_t configlen = _joinpath((const uint8_t**)parts, 2, (uint8_t*)dst, dstlen);
    if (configlen < dstlen-2) {
        dst[configlen++] = SEP;
        dst[configlen] = '\0';
    } else {
        // error: return a minimum effective size
        configlen = strlen(parts[0]) + strlen(parts[1]) + 1;
    }
    return configlen;
}

int roke_get_config_dir(char* dst, size_t dstlen, char* default_path)
{
    size_t len;
    if (default_path != NULL) {
        len = _abspath((uint8_t*) default_path, strlen(default_path),
                       (uint8_t*) dst, dstlen);
        // TODO: this implementation will return at most sizeof(config_dir)-1
        if (len >= dstlen - 2) {
            fprintf(stderr, "config directory path too long.\n");
            return 0;
        }
        dst[len++] = SEP;
        dst[len] = '\0';
    }
    else {
        len = roke_default_config_dir(dst, dstlen);
        if (len >= dstlen) {
            fprintf(stderr, "config directory path too long.\n");
            return 0;
        }
    }
    return len;
}

/**
 * @brief parse a string to retrieve the index and file name
 * @param str a null terminated string to parse
 * @param index updated to contain the parent index
 * @param name  updated to contain the file or directory name
 * @return
 *
 * Each line of the index file contains a three tuple. (index, size, name)
 * parse the line and update the index and name arguments. this function
 * destroys the input str
 */
int
roke_parse_entry(
    uint8_t* str,
    uint32_t* index,
    uint64_t* f_size,
    uint8_t** name)
{
    uint8_t* tmp = str;
    uint8_t* s_index=str;
    uint8_t* s_size=NULL;
    uint8_t* s_name=NULL;

    for (; *tmp!='\0'; tmp++) {
        // find the start of the size component
        if (*tmp==' ' && s_size==NULL) {
            *tmp='\0';
            tmp++;
            s_size=tmp;
        }
        // find the start of the name component
        else if (*tmp==' ' && s_name==NULL) {
            *tmp='\0';
            tmp++;
            s_name=tmp;
        }
        // find the end of the line, the end of the name component
        else if (*tmp=='\n'||*tmp=='\r') {
            *tmp='\0';
            break;
        }
    }

    *index = strtoul((char*)s_index, NULL, 10);
    *f_size = strtoul((char*)s_size, NULL, 10);
    *name = s_name;

    return 0;
}

static int _roke_cancel_build = 0;

int
roke_set_build_cancel_for_test(int value)
{
    int t = _roke_cancel_build;
    _roke_cancel_build = value;
    return t;
}

int
roke_build_cancel()
{
    int t = _roke_cancel_build;
    _roke_cancel_build = 1;
    return t;
}

int
roke_build_index(
    const char* config_dir,
    const char* name,
    const char* root,
    char** blacklist)
{
    return roke_build_index_impl(stdout, config_dir, name, root, blacklist, 0);
}

int
roke_build_index_fd(
    int fd,
    const char* config_dir,
    const char* name,
    const char* root,
    char** blacklist)
{
    FILE* output = roke_fdopen(fd, "w");
    if (output==NULL) {
        fprintf(stderr, "invalid file descriptor: %d\n", fd);
        return -1;
    }

    int err = roke_build_index_impl(output, config_dir, name, root, blacklist, 1);

    fclose(output);

    return err;
}
/**
 * @brief build an index file
 * @param config_dir null terminated string ending in a path separator
 *                   the directory path containing index files
 * @param name      the name of the index file to create or update.
 * @param root      the directory root to begin searching for files
 * @param blacklist null terminated list of strings. directories that exactly
 *                  match a name in this list will not be searched.
 * @return
 *
 * Scan the given directory, and all sub directories for files. build
 * an index file of the directories and files found. When finished, convert
 * the text file into a binary format using roke_binarize_index
 *
 * if the task is aborted the binary index will not be updated, which
 * will prevent persisting a corrupt database
 */
int
roke_build_index_impl(
    FILE* output,
    const char* config_dir,
    const char* name,
    const char* root,
    char** blacklist,
    int verbose)
{
    int i;
    clock_t t_start;
    float elapsed=0;
    float elapsed_total=0;
    uint32_t ndirs=0;
    uint32_t nfiles=0;
    uint8_t didx_path[ROKE_PATH_MAX];
    uint8_t fidx_path[ROKE_PATH_MAX];
    uint8_t eidx_path[ROKE_PATH_MAX];
    uint8_t temp_path[ROKE_PATH_MAX];
    uint8_t idx_name[ROKE_NAME_MAX];
    int _istty;

    FILE *fidx=NULL, *didx=NULL, *eidx=NULL;

    rstack_t stack;

    DIR *d = NULL;
    struct dirent *dir;

    t_start = clock();
    _istty = roke_isatty(roke_fileno(stdout));

    rstack_init(&stack);

    _nstatcalls = 0;

    int aborted = 0;

    if (_roke_cancel_build==1) {
        _roke_cancel_build = 0;
    }

    {
    snprintf((char*) idx_name, sizeof(idx_name), "%s.d.idx", name);
    const uint8_t * parts[] = {(uint8_t*) config_dir, idx_name};
    _joinpath(parts, 2, didx_path, sizeof(didx_path));
    }

    {
    snprintf((char*) idx_name, sizeof(idx_name), "%s.f.idx", name);
    const uint8_t * parts[] = {(uint8_t*) config_dir, idx_name};
    _joinpath(parts, 2, fidx_path, sizeof(fidx_path));
    }

    {
    snprintf((char*) idx_name, sizeof(idx_name), "%s.err", name);
    const uint8_t * parts[] = {(uint8_t*) config_dir, idx_name};
    _joinpath(parts, 2, eidx_path, sizeof(eidx_path));
    }

    fidx = fopen_safe(fidx_path, "w");
    if (fidx == NULL) {
        fprintf(stderr, "failed to open: %s\n", fidx_path);
        goto error;
    }
    didx = fopen_safe(didx_path, "w");
    if (didx == NULL) {
        fprintf(stderr, "failed to open: %s\n", didx_path);
        goto error;
    }
    eidx = fopen_safe(eidx_path, "w");
    if (eidx == NULL) {
        fprintf(stderr, "failed to open: %s\n", eidx_path);
        goto error;
    }

    // write the root index as the first entry in the index
    fprintf(didx, "%d %d %s\n", 0, 0, root);
    ndirs += 1;

    rstack_push(&stack, 0, 0, (uint8_t*) root);

    roke_inode_cache_t cache;
    roke_inode_cache_init(&cache, 1024 * 64 /* * 8 */);

    while (!rstack_empty(&stack)) {

        // the cancel build is implemented as a counter
        // once a thread sets the value every directory processed after it
        // will decrement the counter until the value hits zero, then terminate
        // this is used to implement unit tests with some maximum depth
        if (_roke_cancel_build>0) {
            _roke_cancel_build -= 1;
            if (_roke_cancel_build==0) {
                aborted = 1;
                break;
            }
        }

        rstack_data_t* elem = rstack_head(&stack);
        uint32_t elem_index = elem->index;
        uint32_t elem_depth = elem->depth;
        uint8_t* elem_path  = strdup_safe(elem->path);
        rstack_pop(&stack);

        d = opendir((char*)elem_path);
        if (!d) {
            fprintf(eidx, "failed to open directory: %s\n", elem_path);
            free(elem_path);
            continue;
        }

        while ((dir = readdir(d)) != NULL) {

            // skip blacklisted file names
            int match=0;
            for (i=0; blacklist[i]!=NULL; i++) {
                if (strcmp(blacklist[i], dir->d_name)==0) {

                    match=1;
                    break;
                }
            }
            if (match==1) {
                continue;
            }

            int is_dir  = 0;
            off_t f_size = 0;

            {
                // todo check for errors
                const uint8_t* parts[] = { elem_path, (uint8_t*) dir->d_name };
                _joinpath(parts, 2, temp_path, sizeof(temp_path));
            }

            if (roke_dirent_info(dir, temp_path, &is_dir, &f_size)!=0) {
                fprintf(eidx, "failed to stat path: %s\n", temp_path);
            }

            // minimum file size is 1KB
            // maximum file size is 4096GB, in a 32bit integer
            f_size = (f_size==0) ? 0 : (f_size >> 10 || 1);

            if (is_dir) {
                // write a directory entry to the directory index
                // the entry is [index][size][name]
                // where index is a pointer to the parent

                if (roke_inode_cache_insert(&cache, dir->d_ino)!=ROKE_INODE_CACHE_EXISTS) {

                    fprintf(didx, "%" PFMT_SIZE_T " %" PFMT_SIZE_T " %s\n", (size_t) elem_index, (size_t) f_size, dir->d_name);

                    {
                        // todo check for errors
                        const uint8_t* parts[] = { elem_path, (uint8_t*) dir->d_name };
                        _joinpath(parts, 2, temp_path, sizeof(temp_path));
                    }

                    if (elem_depth < ROKE_RECURSION_DEPTH) {

                        //printf("ino: %d %s\n", dir->d_ino, temp_path);
                        rstack_push(&stack, ndirs, elem_depth + 1, temp_path);
                        ndirs += 1;

                        
                    } else {
                        fprintf(eidx, "recursion depth too deep: %s\n", temp_path);
                    }

                } else {
                    printf("skipping dir: %s\n", temp_path);
                    fprintf(eidx, "skipping dir: %s\n", temp_path);
                }

            } else {
                // write a file entry to the file index
                // the entry is [index][size][name]
                // where index is a pointer to the parent
                fprintf(fidx, "%" PFMT_SIZE_T " %" PFMT_SIZE_T " %s\n", (size_t) elem_index, (size_t) f_size, dir->d_name);
                nfiles += 1;
            }
        }

        free(elem_path);

        if (d != NULL) {
            closedir(d);
        }

        // if the output is a terminal, write the amount of time spent processing
        if (_istty || verbose) {
            elapsed = ((float)(clock() - t_start))/CLOCKS_PER_SEC;

            if (elapsed > (elapsed_total+.5)) {

                fprintf(output,
                    "indexed %d directories and %d files in %f seconds       %c",
                    ndirs, nfiles, elapsed, verbose ? '\n' : '\r');
                fflush(output);
                elapsed_total = elapsed;
            }
        }
    }


  error:
    roke_inode_cache_free(&cache);

    elapsed = ((float)(clock() - t_start))/CLOCKS_PER_SEC;
    fprintf(output,
        "indexed %d directories and %d files in %f seconds      \n",
        ndirs, nfiles, elapsed);

    // todo: verbose should be named prints_status
    if (verbose==0) {
        fprintf(stdout, "stack capacity: %d/%d\n", stack.size, stack.capacity);
    }
    rstack_free(&stack);

    if (fidx != NULL) {
        fclose(fidx);
    }
    if (didx != NULL) {
        fclose(didx);
    }
    if (eidx != NULL) {
        fclose(eidx);
    }

    if (aborted==0) {
        roke_binarize_index(didx_path, ndirs);
        roke_binarize_index(fidx_path, nfiles);
    }

    if (verbose==0) {
        printf("n stat calls: %" PFMT_SIZE_T "\n", _nstatcalls);
    }

    return aborted;
}

/**
 * @brief convert a text index into a binary index.
 * @param index_path the path to an index (.d.idx or .f.idx)
 * @param nitems the number of elements expected to be found in the index file
 * @param is_dir  true if this is a directory index (.d.idx)
 * @return
 *
 * This implementation parses the index files in an on demand process.
 */
int
roke_binarize_index(
    uint8_t* index_path,
    uint32_t nitems)
{
    uint8_t bin_path[ROKE_PATH_MAX];
    uint8_t buffer[ROKE_PATH_MAX];

    size_t n = strcpy_safe(bin_path, sizeof(bin_path), index_path);
    if (n==-1) {
        return 1;
    }
    strcpy_safe(bin_path + n - 3, 4, (uint8_t*)"bin");

    FILE* sidx = NULL;
    FILE* bidx = NULL;

    sidx = fopen_safe(index_path, "r");
    if (sidx == NULL) {
        fprintf(stderr, "failed to open: %s\n", index_path);
        goto error;
    }

    bidx = fopen_safe(bin_path, "wb");
    if (bidx == NULL) {
        fprintf(stderr, "failed to open: %s\n", bin_path);
        goto error;
    }

    // write a header to the binary file so that it can be memmapped easily

    char* headera = "ROKE";
    uint32_t headerb = 0;
    fwrite ( headera,  sizeof(char),     4, bidx);
    fwrite ( &nitems,  sizeof(uint32_t), 1, bidx);
    fwrite ( &headerb, sizeof(uint32_t), 1, bidx);
    fwrite ( &headerb, sizeof(uint32_t), 1, bidx);

    uint32_t elem_offset = 16;
    uint32_t name_offset = 16 + sizeof(roke_entry_t) * nitems;
    uint64_t f_size;
    while (fgets((char*)buffer, sizeof(buffer), sidx) != NULL) {
        roke_entry_t ent;
        uint8_t* name;

        roke_parse_entry(buffer, &ent.index, &f_size, &name);

        ent.namelen = (uint16_t) strlen((char*)name);
        ent.offset = name_offset;
        ent.f_size = f_size; // only valid if ROKE_INDEX_SIZE is true

        fseek(bidx, elem_offset, SEEK_SET);
        fwrite ( &ent, sizeof(roke_entry_t), 1, bidx);
        elem_offset += sizeof(roke_entry_t);

        fseek(bidx, name_offset, SEEK_SET);
        fwrite(name, sizeof(uint8_t), ent.namelen+1, bidx);
        name_offset += ent.namelen+1;
    }

    //fprintf(stderr, "sizeof(roke_entry_t): %d\n", sizeof(roke_entry_t));

  error:
    if (sidx != NULL)
        fclose(sidx);
    if (bidx != NULL)
        fclose(bidx);

    return 0;
}

/**
 * @brief find files patching a given set of patterns
 * @param config_dir null terminated string ending in a path separator
 *                   the directory path containing index files
 * @param patterns   array of at most 2 patterns to use in matching
 *                   the first pattern is used to match file names
 *                   the second pattern is used to match file paths
 * @param npatterns  length of the patterns array
 * @param match_flags set to true to perform case insensitive matching
 * @return
 *
 * This implementation of find memory maps the index files to improve
 * lookup speed by removing the need to parse a text file.
 */
int
roke_locate(
    const char* config_dir,
    const char** patterns,
    size_t npatterns,
    int match_flags,
    int limit)
{
    string_matcher_t smopt1;
    string_matcher_t smopt2;
    string_matcher_t* smopts[3]; // hardcoding to 3 for now...
    int i=0;
    string_matcher_init(&smopt1, (uint8_t*)patterns[0], strlen((char*)patterns[0]), match_flags);

    smopts[0] = &smopt1;
    smopts[1] = NULL;

    if (npatterns > 1) {
        string_matcher_init(&smopt2, (uint8_t*)patterns[1], strlen((char*)patterns[1]), match_flags);
        smopts[1] = &smopt2;
        smopts[2] = NULL;
    }

    int v = roke_locate_impl(stdout, (uint8_t*)config_dir, smopts, limit);

    for (i=0; smopts[i]!=NULL; i++) {
        string_matcher_free(smopts[i]);
    }

    return v;
}

int
roke_locate_fd(
    int fd,
    const char* config_dir,
    const char** patterns,
    size_t npatterns,
    int match_flags,
    int limit)
{

    FILE* output = roke_fdopen(fd, "w");
    if (output==NULL) {
        fprintf(stderr, "invalid file descriptor: %d\n", fd);
        return -1;
    }

    string_matcher_t smopt1;
    string_matcher_t smopt2;
    string_matcher_t* smopts[3]; // hardcoding to 3 for now...
    int i=0;
    string_matcher_init(&smopt1, (uint8_t*)patterns[0], strlen(patterns[0]), match_flags);

    smopts[0] = &smopt1;
    smopts[1] = NULL;

    if (npatterns > 1) {
        string_matcher_init(&smopt2, (uint8_t*)patterns[1], strlen(patterns[1]), match_flags);
        smopts[1] = &smopt2;
        smopts[2] = NULL;
    }

    int v = roke_locate_impl(output, (uint8_t*) config_dir, smopts, limit);

    for (i=0; smopts[i]!=NULL; i++) {
        string_matcher_free(smopts[i]);
    }

    fclose(output);

    return v;
}


int
roke_index_open(roke_index_t* idx, uint8_t* path)
{

    idx->data = NULL;

    idx->fp = fopen_safe(path, "r");
    if (idx->fp == NULL) {
        fprintf(stderr, "error opening %s\n", path);
        return 1;
    }

    fseek(idx->fp, 0, SEEK_END);
    idx->fsize = ftell(idx->fp);
    fseek(idx->fp, 0, SEEK_SET);
    idx->data = mmap(NULL, idx->fsize, PROT_READ, MMAP_FLAGS, roke_fileno(idx->fp), 0);
    if (idx->data == MAP_FAILED) {
        fprintf(stderr, "error mapping %s\n", path);
        goto map_error;
    }

    uint32_t *pnitems = ((uint32_t*)idx->data)+1;
    idx->nitems = *pnitems;

    // get the arrays of entries out of the mem map
    idx->entries = (roke_entry_t*) (((uint8_t*)idx->data)+16);

    // get the initial mem position for which the offset is added
    // to produce a name
    idx->strings = idx->data;

    return 0;

  map_error:
    fclose(idx->fp);
    idx->fp = NULL;

    return 1;

}


int
roke_index_close(roke_index_t* idx)
{
    int rc;

    if (idx->data != NULL) {
        rc = munmap(idx->data, idx->fsize);
        if (rc != 0) {
            fprintf(stderr, "warning: error unmapping directory index.");
        }
    }
    idx->data = NULL;

    if (idx->fp != NULL) {
        fclose(idx->fp);
    }
    idx->fp = NULL;

    return 0;
}


static int roke_locate_index_impl(FILE* output, string_matcher_t** strmatch, roke_index_t* fidx, roke_index_t* didx, int* count, int limit, char* suffix)
{

    uint32_t idx;
    uint8_t buffer1[4096];

    for (idx=0; idx< fidx->nitems; idx++) {

        uint8_t* pname = fidx->strings + fidx->entries[idx].offset;
        uint16_t name_length = fidx->entries[idx].namelen;
        uint32_t parent_index = fidx->entries[idx].index;

        if (string_matcher_match(strmatch[0], pname, name_length)==0) {

            uint8_t* path[ROKE_RECURSION_DEPTH];
            uint32_t path_idx = 1023;
            // copy the path components into the array in reverse order
            path[path_idx--] = pname;
            while (parent_index > 0 && parent_index < didx->nitems) {
                pname = didx->strings + didx->entries[parent_index].offset;
                parent_index = didx->entries[parent_index].index;
                path[path_idx--] = pname;
            }
            pname = didx->strings + didx->entries[0].offset;
            path[path_idx] = pname;

            const uint8_t** parts = (const uint8_t**) (path + path_idx);
            size_t nparts = (sizeof(path) / sizeof(char*)) - path_idx;

            // todo check for errors
            _joinpath(parts, nparts, buffer1, sizeof(buffer1));

            int i, m=0;
            for (i=1; strmatch[i]!=NULL; i++) {
                if (string_matcher_match(strmatch[i],
                        buffer1, sizeof(buffer1))!=0) {
                    m=1;
                    break;
                }
            }
            if (m!=0) {
                continue;
            }

            fprintf(output, "%s%s\n", buffer1, suffix);
            *count++;

            if (limit > 0 && *count >= limit) {
                break;
            }

        }
    }

    return 0;
}

/**
 * @brief find files patching a given set of patterns
 * @param config_dir null terminated string ending in a path separator
 *                   the directory path containing index files
 * @param bmopts     null terminated list of pointers to boyer_moore
 *                   options structs
 * @return
 *
 * This implementation of find memory maps the index files to improve
 * lookup speed by removing the need to parse a text file.
 */
int roke_locate_impl(
    FILE* output,
    const uint8_t* config_dir,
    string_matcher_t** strmatch,
    int limit)
{
    int i=0;
    int count=0;
    uint8_t didx_path[4096];
    uint8_t fidx_path[4096];

    DIR *d = NULL;
    struct dirent *dir;

    roke_index_t didx, fidx;

    d = opendir((char*)config_dir);
    if (!d) {
        fprintf(stderr, "failed to open config directory\n");
        goto error;
    }

    while ((dir = readdir(d)) != NULL) {

        if (limit > 0 && count >= limit) {
            break;
        }

        // find all databases,

        size_t name_len = strlen(dir->d_name);
        if (!has_suffix((uint8_t*) dir->d_name, name_len, (uint8_t*)".d.bin", 6)) {
            continue;
        }

        const uint8_t* parts[] = { config_dir, (uint8_t*)dir->d_name };
        // todo check for errors
        _joinpath(parts, 2, didx_path, sizeof(didx_path));
        strcpy_safe(fidx_path, sizeof(fidx_path), didx_path);
        fidx_path[strlen((char*)fidx_path) - 5] = 'f';

        if (roke_index_open(&didx, didx_path)!=0)
            goto error_didx;

        if (roke_index_open(&fidx, fidx_path)!=0)
            goto error_fidx;

        // match the pattern against directories
        roke_locate_index_impl(output, strmatch, &didx, &didx, &count, limit, "/");

        // match the pattern against files
        roke_locate_index_impl(output, strmatch, &fidx, &didx, &count, limit, "");

    error_fidx:
        roke_index_close(&fidx);
    error_didx:
        roke_index_close(&didx);

    }

  error:

    if (d != NULL) {
        closedir(d);
    }

    return 0;
}

size_t
roke_index_dirinfo(
    char* config_dir,
    char* name,
    /*out*/ char* root,
    size_t rootlen)
{
    uint8_t didx_path[ROKE_PATH_MAX];

    snprintf((char*) didx_path, sizeof(didx_path), "%s%s.d.bin", config_dir, name);

    FILE* didx = fopen_safe(didx_path, "r");

    if (didx == NULL) {
        fprintf(stderr, "failed to open: %s\n", didx_path);
        return 0;
    }

    size_t count;
    roke_entry_t  entry;
    count=fread(root, sizeof(uint8_t), 16, didx);
    if (count != 16) { 
        fclose(didx); 
        fprintf(stderr, "failed to read header\n");
        return 0; 
    }
    count=fread(&entry, sizeof(entry), 1, didx);
    if (count != 1) { 
        fclose(didx); 
        fprintf(stderr, "failed to read entry %d != %d\n", count, sizeof(entry));
        return 0; 
    }
    fseek(didx, entry.offset, SEEK_SET);


    uint32_t index;     // the parent index of this file or directory
    uint64_t f_size;    // file: size of the file, directory: sum of contained files
    uint16_t namelen;   // the length of the file name
    uint32_t offset;    // the offset (from the start of the memory address
                        // where the name can be found.

    count=fread(root, sizeof(uint8_t), entry.namelen + 1, didx);
    if (count != (entry.namelen + 1)) { 
        fclose(didx);
        fprintf(stderr, "failed to read entry root\n");
        return 0; 
    }

    fclose(didx);

    return strlen(root);
}

int roke_index_info(
    char* config_dir,
    char* name,
    /*out*/ uint32_t *ndirs,
    /*out*/ uint32_t *nfiles,
    /*out*/ uint64_t *mtime)
{

    uint8_t didx_path[4096];

    // directory index info

    snprintf((char*)didx_path, sizeof(didx_path), "%s%s.d.bin", config_dir, name);
    FILE* didx = fopen_safe(didx_path, "r");
    if (didx == NULL) {
        fprintf(stderr, "failed to open: %s\n", didx_path);
        return 1;
    }

    size_t count;
    fseek(didx, 4, SEEK_SET);
    count = fread(ndirs, sizeof(uint32_t), 1, didx);
    if (count != 1) { fclose(didx); return 0; }
    fclose(didx);

    // file index info

    snprintf((char*)didx_path, sizeof(didx_path), "%s%s.f.bin", config_dir, name);
    FILE* fidx = fopen_safe(didx_path, "r");
    if (fidx == NULL) {
        fprintf(stderr, "failed to open: %s\n", didx_path);
        return 1;
    }
    fseek(fidx, 4, SEEK_SET);
    count = fread(nfiles, sizeof(uint32_t), 1, fidx);
    if (count != 1) { fclose(fidx); return 0; }
    fclose(fidx);

    *mtime = creation_time(didx_path);

    return 0;
}

/**
 * @brief rebuild the named index file found in the config directory
 * @param config_dir null terminated string ending in a path separator
 *                   the directory path containing index files
 * @param blacklist null terminated list of strings. directories that exactly
 *                  match a name in this list will not be searched.
 * @return
 *
 */
int
roke_rebuild_index(
    char* config_dir,
    char* name,
    char** blacklist)
{
    char root[ROKE_PATH_MAX];

    roke_index_dirinfo(config_dir, name, root, sizeof(root));

    fprintf(stdout, "Rebuilding  %s: root=%s\n", name, root);

    return roke_build_index(config_dir, name, root, blacklist);
}