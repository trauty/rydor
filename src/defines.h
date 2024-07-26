#pragma once

#include <math.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float_t f32;
typedef double_t f64;

static_assert(sizeof(u8) == 1, "STATIC ASSERT FAILED: u8 not 1 byte wide");
static_assert(sizeof(u16) == 2, "STATIC ASSERT FAILED: u16 not 2 byte wide");
static_assert(sizeof(u32) == 4, "STATIC ASSERT FAILED: u32 not 4 byte wide");
static_assert(sizeof(u64) == 8, "STATIC ASSERT FAILED: u64 not 8 byte wide");

static_assert(sizeof(i8) == 1, "STATIC ASSERT FAILED: i8 not 1 byte wide");
static_assert(sizeof(i16) == 2, "STATIC ASSERT FAILED: i16 not 2 byte wide");
static_assert(sizeof(i32) == 4, "STATIC ASSERT FAILED: i32 not 4 byte wide");
static_assert(sizeof(i64) == 8, "STATIC ASSERT FAILED: i64 not 8 byte wide");