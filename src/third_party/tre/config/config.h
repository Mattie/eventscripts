#ifndef ES_TRE_BUILD_CONFIG_H
#define ES_TRE_BUILD_CONFIG_H
#include "tre-config.h"
#define STDC_HEADERS 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MALLOC_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define TRE_REGEX_T_FIELD value
#if defined(_MSC_VER)
#define inline __inline
#if _MSC_VER < 1900
#define snprintf sprintf_s
#endif
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#ifndef _WIN32
#define HAVE_ALLOCA_H 1
#define HAVE_ISBLANK 1
#endif
#endif
