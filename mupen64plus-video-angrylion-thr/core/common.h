#pragma once

#include <stdint.h>

// inlining
#define INLINE inline

#ifdef _MSC_VER
#define STRICTINLINE __forceinline
#else
#define STRICTINLINE INLINE
#endif

/* RGBA5551 to RGBA8888 helper */
#define RGBA16_R(x)  (((x) >> 8) & 0xf8)
#define RGBA16_G(x)  (((x) & 0x7c0) >> 3)
#define RGBA16_B(x)  (((x) & 0x3e) << 2)

/* RGBA8888 helper */
#define RGBA32_R(x) (((x) >> 24) & 0xff)
#define RGBA32_G(x) (((x) >> 16) & 0xff)
#define RGBA32_B(x) (((x) >> 8) & 0xff)
#define RGBA32_A(x) ((x) & 0xff)

int32_t irand(void);
