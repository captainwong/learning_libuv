#ifndef __LIB_UV_H__
#define __LIB_UV_H__

#pragma once

#include "G:/dev_libs/libuv-1.44.2/include/uv.h"

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "G:/dev_libs/libuv-1.44.2/build/Debug/uv.lib")
#else
#pragma comment(lib, "G:/dev_libs/libuv-1.44.2/build/Release/uv.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "G:/dev_libs/libuv-1.44.2/build-x86/Debug/uv.lib")
#else
#pragma comment(lib, "G:/dev_libs/libuv-1.44.2/build-x86/Release/uv.lib")
#endif
#endif

#endif 
