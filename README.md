##Overview

```libqtxdg``` is a Qt implementation of freedesktop.org XDG specifications which is built with Qt5.

##Dependencies

   - Qt5

##Configuration

```libqtxdg``` uses the CMake build system. Everything that applies to CMake also
applies here.

###Configuration options:
    BUILD_TESTS         Builds tests, defaults to OFF

###Configuration Examples:
Build library  and build self tests: ```cmake -DBUILD_TESTS=ON ..```

Build the library without building self tests : ```cmake ..```
