
# Roke

> An isle that cannot be found, Roke
> - The Finder, Tales From Earthsea by Ursula Le Guin

Roke is a fast alternative to updatedb/locate with both a cli and gui.

Supports Linux, Mac OS, and Windows (MSVC 2015 and MSYS2).
File systems which do not support dirent.d_type will be significantly
slower to index

## Build C Components

### Linux and OSX

Run CMake to generate a build directory. A helper script is included in the util/ directory to aid in runnning CMake. The script takes an optional argument which will determine the type of build directory made.

    ./util/run_cmake.sh [release|debug|coverage|profile]
    cd release
    make
    make install

* release
  * Create an optimized build
* debug
  * Build with debug symbols
* coverage
  * Adds an additional build target "coverage" which will generate an html coverage report using gcov and locv.
* profile
  * Adds additional build targets "profile-roke" and "profile-roke-build" which will build executables with the same name. these targets will generate runtime profile information for gprof.


### Windows

Run CMake to generate a build directory. A helper script is included for MSVC 2015.

    util\run_cmake_msvc.bat

## Build Python

Once a release build has been made for your platform, the python gui can built.


    python3 -m pip install -r requirements.txt
    python3 setup.py install
    python3 setup.py run
    python3 setup.py freeze

Usage:

Installing Roke installs the falling binaries.
See the help menu for more information about each command

- `roke-build` - build or create an index
- `roke-list` - list metadata about indexes
- `roke-refresh` - rebuild indexes
- `roke` - search files by name
