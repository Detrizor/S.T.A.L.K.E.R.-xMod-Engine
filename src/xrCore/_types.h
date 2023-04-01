#pragma once

// Type defs
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed __int64 s64;
typedef unsigned __int64 u64;

typedef float f32;
typedef double f64;

typedef char* pstr;
typedef const char* pcstr;

// windoze stuff
#ifndef _WINDOWS_
typedef int BOOL;
typedef pstr LPSTR;
typedef pcstr LPCSTR;
#define TRUE true
#define FALSE false
#endif

// Type limits
#define type_max(T) (::std::numeric_limits<T>::max)()
#define type_min(T) -(::std::numeric_limits<T>::max)()
#define type_zero(T) (::std::numeric_limits<T>::min)()
#define type_epsilon(T) (::std::numeric_limits<T>::epsilon)()

constexpr int int_max		= type_max(int);
constexpr int int_min		= type_min(int);
constexpr int int_zero		= type_zero(int);

constexpr u16 u16_max		= type_max(u16);

constexpr float flt_max		= type_max(float);
constexpr float flt_min		= type_min(float);
constexpr float FLT_MAX		= flt_max;
constexpr float FLT_MIN		= flt_min;

constexpr float flt_zero	= type_zero(float);
constexpr float flt_eps		= type_epsilon(float);

constexpr double dbl_max		= type_max(double);
constexpr double dbl_min		= type_min(double);
constexpr double dbl_zero		= type_zero(double);
constexpr double dbl_eps		= type_epsilon(double);

typedef char string16[16];
typedef char string32[32];
typedef char string64[64];
typedef char string128[128];
typedef char string256[256];
typedef char string512[512];
typedef char string1024[1024];
typedef char string2048[2048];
typedef char string4096[4096];

typedef char string_path[2 * _MAX_PATH];
