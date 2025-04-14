#pragma once

#include <math.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;

typedef int8_t i8_t;
typedef int16_t i16_t;
typedef int32_t i32_t;
typedef int64_t i64_t;

typedef float f32;
typedef double f64;

static_assert(sizeof(u8) == 1, "STATIC ASSERT FAILED: u8 not 1 byte wide");
static_assert(sizeof(u16) == 2, "STATIC ASSERT FAILED: u16 not 2 byte wide");
static_assert(sizeof(u32) == 4, "STATIC ASSERT FAILED: u32 not 4 byte wide");
static_assert(sizeof(u64) == 8, "STATIC ASSERT FAILED: u64 not 8 byte wide");

static_assert(sizeof(i8) == 1, "STATIC ASSERT FAILED: i8 not 1 byte wide");
static_assert(sizeof(i16) == 2, "STATIC ASSERT FAILED: i16 not 2 byte wide");
static_assert(sizeof(i32) == 4, "STATIC ASSERT FAILED: i32 not 4 byte wide");
static_assert(sizeof(i64) == 8, "STATIC ASSERT FAILED: i64 not 8 byte wide");

#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#endif