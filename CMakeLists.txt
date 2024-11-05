cmake_minimum_required(VERSION 3.15)

# unit tests should print #run #failed #skipped

# test unicode search on windows from the hui

# ripgrep
# https://boyter.org/posts/sloc-cloc-code/
# https://techoverflow.net/2013/08/21/a-simple-mmap-readonly-example/
# https://mirrors.edge.kernel.org/pub/linux/kernel/people/geoff/cell/ps3-linux-docs/CellProgrammingTutorial/BasicsOfSIMDProgramming.html
# https://en.wikipedia.org/wiki/SIMD

#  'ЗДРАВСТВУЙ МИР!'
# b'\xd0\x97\xd0\x94\xd0\xa0\xd0\x90\xd0\x92\xd0\xa1\xd0\xa2\xd0\x92\xd0\xa3\xd0\x99\x20\xd0\x9c\xd0\x98\xd0\xa0!'
#  'здравствуй мир!'
# b'\xd0\xb7\xd0\xb4\xd1\x80\xd0\xb0\xd0\xb2\xd1\x81\xd1\x82\xd0\xb2\xd1\x83\xd0\xb9\x20\xd0\xbc\xd0\xb8\xd1\x80!'
# dangergang
# b'\xe3\x83\x87\xe3\x83\xb3\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xbc\xe2\x98\x86\xe3\x82\xae\xe3\x83\xa3\xe3\x83\xb3\xe3\x82\xb0'
# danger
# b'\xe3\x83\x87\xe3\x83\xb3\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xbc'
# gang
# b'\xe3\x82\xae\xe3\x83\xa3\xe3\x83\xb3\xe3\x82\xb0'


project(roke
    VERSION 0.1.0
    LANGUAGES C)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/util")

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
        add_compile_options("-g")
    endif()
else()
    if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
        add_compile_options("-g")
    else()
        add_compile_options("-O3")
    endif()
    #add_compile_options("-Wno-pointer-sign")
    add_compile_options("-Wall")
endif()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message("build type is: ${CMAKE_BUILD_TYPE} : ${CMAKE_C_FLAGS}")

set(ROKE_SRC "${PROJECT_SOURCE_DIR}/src")

if(${ROKE_COVERAGE})
  message("Setting up test coverage")
  set(CMAKE_C_FLAGS "-DDEBUG -g -O0 --coverage -fprofile-arcs -ftest-coverage ${CMAKE_C_FLAGS}" CACHE INTERNAL "" FORCE)
  include(CMakeCover)
endif()

include_directories( ${ROKE_SRC} )

# ---------------------------------------------------------
# libtrex

set(trex_src
        ${ROKE_SRC}/trex/trex.c
        ${ROKE_SRC}/trex/trex.h
        )
add_library(trex STATIC ${trex_src})

# ---------------------------------------------------------
# libutf8proc

set(utf8proc_src
    ${ROKE_SRC}/utf8/utf8proc.c
    ${ROKE_SRC}/utf8/utf8proc.h
    ${ROKE_SRC}/utf8/utf8proc_data.h
)
add_library(utf8proc STATIC ${utf8proc_src})

# ---------------------------------------------------------
# libroke

set(libroke_src

    ${ROKE_SRC}/roke/common/argparse.c
    ${ROKE_SRC}/roke/common/argparse.h
    ${ROKE_SRC}/roke/common/boyer_moore.c
    ${ROKE_SRC}/roke/common/boyer_moore.h
    ${ROKE_SRC}/roke/common/compat.h
    ${ROKE_SRC}/roke/common/cache.c
    ${ROKE_SRC}/roke/common/cache.h
    ${ROKE_SRC}/roke/common/pathutil.c
    ${ROKE_SRC}/roke/common/pathutil.h
    ${ROKE_SRC}/roke/common/stack.c
    ${ROKE_SRC}/roke/common/stack.h
    ${ROKE_SRC}/roke/common/unittest.c
    ${ROKE_SRC}/roke/common/unittest.h
    ${ROKE_SRC}/roke/common/strutil.c
    ${ROKE_SRC}/roke/common/strutil.h
    ${ROKE_SRC}/roke/common/regex.c
    ${ROKE_SRC}/roke/common/regex.h
    ${ROKE_SRC}/roke/libroke.h
    ${ROKE_SRC}/roke/libroke_internal.h
    ${ROKE_SRC}/roke/libroke.c
    )

if (MINGW)
  set(libroke_src ${libroke_src} ${ROKE_SRC}/dirent/dirent_msys.h
                                 ${ROKE_SRC}/dirent/dirent_msys.c)
elseif (WIN32)
  set(libroke_src ${libroke_src} ${ROKE_SRC}/dirent/dirent.h
                                 ${ROKE_SRC}/dirent/dirent.c)
endif()

if(${_DIRENT_HAVE_D_TYPE})
    add_definitions("-D_DIRENT_HAVE_D_TYPE=1")
endif()

add_library(libroke SHARED ${libroke_src})
set_target_properties(libroke
                      PROPERTIES OUTPUT_NAME "roke")

TARGET_LINK_LIBRARIES(libroke utf8proc)
TARGET_LINK_LIBRARIES(libroke trex)

IF (WIN32)
    # needed to export symbols for windows dll.
    target_compile_options(libroke
            PRIVATE "-Droke_EXPORTS=1")
endif()

IF (WIN32 OR MINGW)
    # on windows, MSYS2 does not have sys/mman
    # mman-win32 has been included as part of the source
    # https://github.com/mcgarrah/mman-win32
    set(mman_src
        ${ROKE_SRC}/mman/mman.c
        ${ROKE_SRC}/mman/mman.h
        )
    add_library(mman STATIC ${mman_src})
    IF (WIN32)
        target_compile_options(mman
            PRIVATE "/Oy")
    else()
        target_compile_options(mman
            PRIVATE "-fomit-frame-pointer")
    endif()
    TARGET_LINK_LIBRARIES(libroke mman)
endif()



function(build_roke_binary name main)
    add_executable(${name}
        ${main}
        ${ROKE_SRC}/roke/libroke.h
        ${ROKE_SRC}/roke/libroke_internal.h)
    TARGET_LINK_LIBRARIES(${name} libroke)
    set_target_properties(${name} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib"
    )

endfunction()


build_roke_binary("roke"         ${ROKE_SRC}/roke/bin/locate.c)
build_roke_binary("roke-build"   ${ROKE_SRC}/roke/bin/build.c)
build_roke_binary("roke-refresh" ${ROKE_SRC}/roke/bin/refresh.c)
build_roke_binary("roke-list"    ${ROKE_SRC}/roke/bin/list.c)


if(${ROKE_PROFILE})
    add_executable("profile-roke" ${ROKE_SRC}/roke/bin/locate.c ${src})
    target_compile_options("profile-roke" PRIVATE "-pg")
    set_target_properties("profile-roke" PROPERTIES LINK_FLAGS "-pg")

    add_executable("profile-roke-build" ${ROKE_SRC}/roke/bin/build.c ${src})
    target_compile_options("profile-roke-build" PRIVATE "-pg")
    set_target_properties("profile-roke-build" PROPERTIES LINK_FLAGS "-pg")

endif()

# ---------------------------------------------------------
# unit tests

function(build_roke_test name main)
    add_executable("test-${name}"
        ${main}
        ${ROKE_SRC}/roke/libroke.h
        ${ROKE_SRC}/roke/libroke_internal.h)
    TARGET_LINK_LIBRARIES("test-${name}" libroke)
    add_test(NAME "test-${name}"
        COMMAND "test-${name}" ${ARGN}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    set_target_properties("test-${name}" PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endfunction()

build_roke_test("unittest"    ${ROKE_SRC}/roke/common/unittest_test.c)
build_roke_test("argparse"    ${ROKE_SRC}/roke/common/argparse_test.c)
build_roke_test("strutil"     ${ROKE_SRC}/roke/common/strutil_test.c)
build_roke_test("pathutil"    ${ROKE_SRC}/roke/common/pathutil_test.c)
build_roke_test("boyer-moore" ${ROKE_SRC}/roke/common/boyer_moore_test.c)
build_roke_test("regex"       ${ROKE_SRC}/roke/common/regex_test.c)
build_roke_test("stack"       ${ROKE_SRC}/roke/common/stack_test.c)
build_roke_test("dirent"      ${ROKE_SRC}/dirent/dirent_test.c
                              ${PROJECT_SOURCE_DIR}/test/resource)

build_roke_test("libroke"     ${ROKE_SRC}/roke/libroke_test.c
    ${PROJECT_SOURCE_DIR}/test/config ${PROJECT_SOURCE_DIR}/src)

# ---------------------------------------------------------
# integation tests

set(test_bin_dir "${CMAKE_BINARY_DIR}/bin")
if(WIN32)
set(test_bin_dir "${CMAKE_BINARY_DIR}/bin/Release")
endif()

add_test(NAME test-integration
         COMMAND python "${PROJECT_SOURCE_DIR}/test/integration.py"
                        "${test_bin_dir}"
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# ---------------------------------------------------------
# install

if (NOT MSVC)
    install(TARGETS libroke
            DESTINATION /usr/local/lib)

    install(TARGETS "roke" "roke-build" "roke-refresh" "roke-list"
            DESTINATION /usr/local/bin)
endif()