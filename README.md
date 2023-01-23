# libuv notes

## Linux Build

`cd` to `path/to/libuv`

`mkdir build && cd build`

`cmake .. -DBUILD_TESTING=ON`

`cmake --build .` or `make`

## Windows Build

### x64

open `x64 Native Tools Command Prompt for VS 2019`

`cd` to `path/to/libuv`

`mkdir build && cd build`

`cmake .. -DBUILD_TESTING=ON`

`cmake --build .` or open `libuv.sln` to build.

### Win32

open `x86 Native Tools Command Prompt for VS 2019`

`cd` to `path/to/libuv`

`mkdir build-x86 && cd build-x86`

`cmake .. -DBUILD_TESTING=ON -DCMAKE_GENERATOR_PLATFORM=Win32`

`cmake --build .` or open `libuv.sln` to build.

