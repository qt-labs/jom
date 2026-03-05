# jom

jom is a parallel make tool for Windows. It is an nmake clone with
support for parallel builds.

## Building with QMake

```bat
qmake
nmake
```

### Running the tests

```bat
nmake check
```

## Building with CMake

CMake builds must use a separate build directory.

```bat
mkdir build
cd build
cmake ..\jom -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=<qt-path> -DCMAKE_INSTALL_PREFIX=<install-path>
nmake
nmake install
```

### Running the tests

Enable tests during configuration:

```bat
cmake ..\jom -G "NMake Makefiles" -DBUILD_TESTING=ON
nmake
ctest -V
```

## Environment variables

Like nmake, jom reads default command line arguments from an environment
variable: `JOMFLAGS`. If `JOMFLAGS` is not set, `MAKEFLAGS` is read.
This is useful to set up separate flags for nmake and jom, e.g.

```bat
set MAKEFLAGS=L
set JOMFLAGS=Lj8
```

## .SYNC dependents

The `.SYNC` directive on the right side of a description block prevents
jom from running all of its dependents in parallel.

```makefile
all: Init Prebuild .SYNC Build .SYNC Postbuild
```

This adds additional dependencies so that `Init` and `Prebuild` are
built before `Build`, and `Build` is built before `Postbuild`.
