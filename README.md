##Overview

```libqtxdg``` is a Qt implementation of freedesktop.org XDG specifications which is built with Qt5.

##Dependencies

   - Qt5

   ###Runtime optional dependencies:
   * gtk-update-icon-cache
     * **Note:** We are able now to use GTK+ icon theme caches for faster icon lookup. The cache file can be generated with gtk-update-icon-cache utility on a theme directory. If the cache is not present, corrupted, or outdated, the normal slow lookup is still run.


##Configuration

```libqtxdg``` uses the CMake build system. Everything that applies to CMake also
applies here.

###Configuration options:
    BUILD_TESTS         Builds tests, defaults to OFF
    BUILD_DEV_UTILS     Builds and install development utils, defaults to OFF

###Configuration Examples:
Build library  and build self tests: ```cmake -DBUILD_TESTS=ON ..```

Build the library without building self tests : ```cmake ..```
