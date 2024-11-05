import os, sys, ctypes
import platform
import threading

"""
expose an implementation which accepts a writer fd, using os.pipe

the python code can then read matches out line by line in
"""
"""
rd, wd = os.pipe()

rb = os.fdopen(rd, "rb")
wb = os.fdopen(wd, "wb")

wb.write("hello\n")
wb.flush()
print(rb.readline())
wb.write("hello\n")
wb.flush()
print(rb.readline())
"""

def _buildArtifact(so_name, platname):

    if platname == "windows":
        build_path = os.path.join(os.getcwd(), "MSVC", "bin", "Release", so_name)
    elif platname == "darwin":
        build_path = os.path.join(os.getcwd(), "release-osx", "lib", so_name)
    else:

        build_path = os.path.join(os.getcwd(), "build", "lib", so_name)
        if not os.path.exists(build_path):
            build_path = os.path.join(os.getcwd(), "debug", "lib", so_name)

    return build_path;

def BuildArtifact(libname):
    platname = platform.system().lower()

    if platname == 'windows':
        so_name = "%s.dll"%libname
    elif platname == 'darwin':
        so_name = "lib%s.dylib"%libname
    else:
        so_name = "lib%s.so"%libname

    return _buildArtifact(so_name, platname)

def _findLibrary(so_name, platname):
    path=None
    pwd_path = os.path.join(os.getcwd(), so_name)
    inst_path = os.path.dirname(os.path.abspath(__file__))
    inst_path = os.path.join(inst_path, so_name)

    build_path = _buildArtifact(so_name, platname)

    if os.path.exists( pwd_path ):
        path = pwd_path
    elif hasattr(sys, '_MEIPASS'):
        path= os.path.join(sys._MEIPASS, so_name)
        if not os.path.exists(path):
            path= os.path.join(sys._MEIPASS, "pyroke", so_name)
    elif '_MEIPASS2' in os.environ:
        path= os.path.join(os.environ['_MEIPASS2'], so_name)
        if not os.path.exists(path):
            path= os.path.join(os.environ['_MEIPASS2'], "pyroke", so_name)
    elif os.path.exists(inst_path):
        path = inst_path
    elif os.path.exists(build_path):
        path = build_path
    elif os.path.exists("/usr/local/lib/%s"%so_name):
        path = "/usr/local/lib/%s"%so_name
    else:
        raise RuntimeError("not Found: %s"%(so_name))
    return path

    if os.path.exists(path):
        dpath = os.path.dirname(os.path.abspath(path))
        if dpath not in os.environ["PATH"]:
            os.environ["PATH"] += os.pathsep + dpath
    print("libebfs: %s" % path)
    return path

def FindLibrary(libname):
    platname = platform.system().lower()

    if platname == 'windows':
        so_name = "%s.dll"%libname
    elif platname == 'darwin':
        so_name = "lib%s.dylib"%libname
    else:
        so_name = "lib%s.so"%libname

    return _findLibrary(so_name, platname)

def LoadLibrary(libname):
    """
        load a runtime-linked library (.dll/.so) given by "libname"
        tries to look in a few common areas first.
        returns a ctypes handle to the dll and a version
        specific func type. (windows/Linux)
    """
    platname = platform.system().lower()
    path = FindLibrary(libname)
    print(path)

    if platname == 'windows':
        module = ctypes.WinDLL( path )
        func_type = ctypes.WINFUNCTYPE
    else:
        module = ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL)
        func_type = ctypes.CFUNCTYPE
    return module, func_type

module,func_type = LoadLibrary("roke")

def chkcall(fptr,name,*args):
    result=fptr(*args)
    if result!=0:
        sargs = ','.join(['%s'%s for s in args])
        sresult = errno.errorcode.get(result,"Unknown Error")
        raise EBFSError("%s(%s) returned %2d '%s'"%(name,sargs,result,sresult))

def cdef(name,return_type,*args):
    sys.modules[__name__].__dict__[name] = \
        func_type(return_type,*args)((name,module))

def cdef_chk(name,return_type,*args):
    """define a c-function which raises an exception on error"""
    func = func_type(return_type,*args)((name,module))
    sys.modules[__name__].__dict__[name] = lambda *A : chkcall(func,name,*A);

#------------------------------------------------------------------------------
# definitions for c types

c_void_p   = ctypes.c_void_p
c_bool     = ctypes.c_bool
c_int      = ctypes.c_int
c_int_p    = ctypes.POINTER(ctypes.c_int)
c_long     = ctypes.c_long
c_size_t   = ctypes.c_size_t
c_size_t_p = ctypes.POINTER(ctypes.c_size_t)
c_char_p   = ctypes.c_char_p
c_char_pp  = ctypes.POINTER(c_char_p)
c_uint8    = ctypes.c_uint8
c_uint8_p  = ctypes.POINTER(ctypes.c_uint8)
c_uint32   = ctypes.c_uint32
c_uint32_p = ctypes.POINTER(ctypes.c_uint32)
c_uint64   = ctypes.c_uint64
c_uint64_p = ctypes.POINTER(ctypes.c_uint64)

#------------------------------------------------------------------------------
# definitions for roke types

ROKE_CASE_SENSITIVE = 0
ROKE_CASE_INSENSITIVE = 1
ROKE_DEFAULT = 0
ROKE_GLOB = 2
ROKE_REGEX = 4
ROKE_MATCH_MASK = (ROKE_GLOB|ROKE_REGEX)

cdef('roke_default_config_dir', c_uint64, c_char_p, c_uint64)
cdef('roke_build_index_fd', c_int, c_int, c_char_p, c_char_p, c_char_p, c_char_pp)
cdef('roke_locate_fd', c_int, c_int, c_char_p, c_char_pp, c_uint64, c_int, c_int)
cdef('roke_build_cancel', c_int)
cdef('roke_index_dirinfo', c_int, c_char_p, c_char_p, c_char_p, c_size_t)
cdef('roke_index_info', c_int, c_char_p, c_char_p, c_uint32_p, c_uint32_p, c_uint64_p)


class RokeException(Exception):
    pass


def index_info(config_dir, name):

    enc_cfg  = config_dir.encode("utf-8")
    enc_name = name.encode("utf-8")
    rdbufsize = 4096
    buf = (ctypes.c_char * rdbufsize)()
    ndirs = c_uint32()
    nfiles = c_uint32()
    mtime = c_uint64()

    count = roke_index_dirinfo(enc_cfg, enc_name, buf, rdbufsize)
    root = ""
    if 0 < count < rdbufsize:
        root = buf[:count].decode("utf-8")

    error = roke_index_info(enc_cfg, enc_name,
        ctypes.byref(ndirs), ctypes.byref(nfiles), ctypes.byref(mtime))

    st = lambda:None
    st.root = root
    st.mtime = mtime.value
    st.ndirs = ndirs.value
    st.nfiles = nfiles.value

    return st

def default_config_dir():

    rdbufsize = 4096
    buf = (ctypes.c_char * rdbufsize)()

    count = roke_default_config_dir(buf, rdbufsize)
    if 0 < count < rdbufsize:
        return buf[:count].decode("utf-8")
    raise Exception("error getting default config directory: %d" % count)

def _find_impl(result, fd, config_dir, patterns, flags, limit):

    try:
        if isinstance(patterns, str):
            patterns = [patterns, ]

        enc_dir = config_dir.encode("utf-8")

        patternlen = len(patterns)
        patterns = [x.encode("utf-8") for x in patterns]
        data = (c_char_p *(len(patterns)))(*patterns)

        count = roke_locate_fd(fd, enc_dir, data, patternlen, flags, limit)

        result['count'] = count
        result['error'] = None
    except Exception as e:
        # prevents a hang, waiting for fd to close
        os.close(fd)
        result['count'] = 0
        result['error'] = "%s" % e

def find(config_dir, patterns, flags=ROKE_CASE_SENSITIVE, limit=1000):

    rd, wd = os.pipe()

    result = {'count': 0}

    rb = os.fdopen(rd, "rb")
    t = threading.Thread(target=_find_impl,
        args=(result, wd, config_dir, patterns, flags, limit))

    try:
        t.start()


        for line in rb:
            try:
                yield line.decode("utf-8", "ignore")[:-1]
            except:
                print(line)

    finally:
        rb.close()
        t.join()

    if result['count'] < 0:
        raise RokeException("Error executing query: %s" % patterns)

    if result['error'] is not None:
        raise RokeException("Locate Failed: %s" % result['error'])

def _build_impl(result, fd, config_dir, name, root):

    try:
        enc_dir = config_dir.encode("utf-8")
        enc_name = name.encode("utf-8")
        enc_path = root.encode("utf-8")

        blacklist = [".", "..", ".git", ".svn", ".dropbox", ".dropbox.cache"]
        blacklist = [x.encode("utf-8") for x in blacklist] + [None]
        blacklist = (c_char_p*(len(blacklist)))(*blacklist)

        status = roke_build_index_fd(fd, enc_dir, enc_name, enc_path, blacklist)

        result['status'] = status
        result['error'] = None
    except Exception as e:
        # prevents a hang, waiting for fd to close
        os.close(fd)
        result['status'] = 0
        result['error'] = "%s" % e

def build(config_dir, name, path):


    rd, wd = os.pipe()

    result = {"status":0}

    rb = os.fdopen(rd, "rb")
    t = threading.Thread(target=_build_impl,
        args=(result, wd, config_dir, name, path))

    try:
        t.start()

        for line in rb:
            try:
                yield line.decode("utf-8", "ignore")[:-1]
            except:
                print(line)

    finally:
        rb.close()
        t.join()

    if result['status'] != 0:
        raise RokeException("Build Failed")

    if result['error'] is not None:
        raise RokeException("Build Failed: %s" % result['error'])

def rebuild(config_dir, name):

    st = index_info(config_dir, name)

    return build(config_dir, name, st.root)

def build_cancel():
    roke_build_cancel()

def rename_index(config_dir, old_name, new_name):

    exts = [".d.idx", ".d.bin",
            ".f.idx", ".f.bin", ".err"]

    # check that the target name does not exist
    for ext in exts:
        path = os.path.join(config_dir, new_name + ext)
        if os.path.exists(path):
            return False

    for ext in exts:
        old_path = os.path.join(config_dir, old_name + ext)
        new_path = os.path.join(config_dir, new_name + ext)
        if os.path.exists(old_path):
            os.rename(old_path, new_path)

    return True






