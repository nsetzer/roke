
mode="${1:-release}"

USE_COVERAGE=
USE_PROFILE=

case "$mode" in
    release)
        BUILD_TYPE=Release
    ;;
    debug)
        BUILD_TYPE=Debug
    ;;
    profile)
        BUILD_TYPE=Debug
        USE_PROFILE="-DROKE_PROFILE=1"
    ;;
    coverage)
        BUILD_TYPE=Debug
        USE_COVERAGE="-DROKE_COVERAGE=1"
    ;;
    *)
        echo "unknown profile: $mode"
        echo
        echo "usage: $0 {release|debug|profile|coverage}"
        exit 1
    ;;
esac

export CMAKE_C_COMPILER_ENV_VAR="%CMAKE_C_COMPILER_ENV_VAR%"
export CMAKE_CXX_COMPILER_ENV_VAR="%CMAKE_CXX_COMPILER_ENV_VAR%"

export CMAKE_C_COMPILER="`which gcc`"
export CMAKE_CXX_COMPILER="`which g++`"

GCOV_PATH="`which gcov`"

devtools_dir=/opt/rh/devtoolset-2/root/
echo "Checking existence of $devtools_dir... "
if [[ -d $devtools_dir ]]; then
    GCC_PATH=/opt/rh/devtoolset-2/root/usr/bin/gcc
    GXX_PATH=/opt/rh/devtoolset-2/root/usr/bin/g++
    GCOV_PATH=/opt/rh/devtoolset-2/root/usr/bin/gcov
else
    export GCC_PATH="$CMAKE_C_COMPILER"
    export GXX_PATH="$CMAKE_CXX_COMPILER"
fi

export CC="$GCC_PATH"
export CXX="$GXX_PATH"

CMAKE_LEGACY_CYGWIN_WIN32=0
#export CMAKE_LEGACY_CYGWIN_WIN32=0

echo $CC
echo $CXX
echo "$GCOV_PATH"
# change the directory name when running under cygwin,
# so that mingw and cygwin can live side by side
platname=
[[ "$(uname)" = "CYGWIN"* ]] && platname=-cygwin
[[ "$(uname)" = "MSYS"* ]] && platname=-msys
[[ "$(uname)" = "MINGW"* ]] && platname=-mingw
[[ "$(uname)" = "Darwin"* ]] && platname=-osx

[ -d ./${mode}${platname} ] || mkdir ./${mode}${platname}

# compile a sample program to see if struct dirent
# has the d_type field.
HAVE_DIRENT=
if bash ./util/test_dirent.sh $CC; then
    HAVE_DIRENT="-D_DIRENT_HAVE_D_TYPE=1"
fi

cmake=`which cmake 2>/dev/null || which cmake3`
cd ./${mode}${platname}
$cmake -G "Unix Makefiles" \
    ${HAVE_DIRENT} \
    ${USE_COVERAGE} \
    ${USE_PROFILE} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_MACOSX_RPATH=NEW \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DGCOV_PATH="$GCOV_PATH" \
    ..

