/**
 * @file
 */

#pragma once

#include <string.h>

#ifndef SDLCALL
#if defined(_WIN32) && !defined(__GNUC__)
#define SDLCALL __cdecl
#else
#define SDLCALL
#endif
#endif

extern "C" void *SDLCALL SDL_malloc(size_t size);
extern "C" void *SDLCALL SDL_realloc(void *mem, size_t size);
extern "C" void SDLCALL SDL_free(void *mem);
extern "C" void *SDLCALL SDL_memset(void *dst, int c, size_t len);
extern "C" void *SDLCALL SDL_memcpy(void *dst, const void *src, size_t len);
extern "C" int SDLCALL SDL_memcmp(const void *s1, const void *s2, size_t len);
extern "C" char *SDLCALL SDL_strdup(const char *str);

#ifdef _MSC_VER
#define core_strcasecmp _stricmp
#else
#define core_strcasecmp strcasecmp
#endif

#ifndef core_malloc
#define core_malloc SDL_malloc
#endif

#ifndef core_realloc
#define core_realloc SDL_realloc
#endif

#ifndef core_free
#define core_free SDL_free
#endif

#ifndef core_strdup
#define core_strdup SDL_strdup
#endif

#ifndef core_memset
#define core_memset SDL_memset
#endif

#ifndef core_memcpy
#define core_memcpy SDL_memcpy
#endif

#ifndef core_memcmp
#define core_memcmp SDL_memcmp
#endif

#ifndef core_zero
#define core_zero core_memset(&(x), 0, sizeof((x)))
#endif

#ifndef core_zerop
#define core_zerop core_memset((x), 0, sizeof(*(x)))
#endif
