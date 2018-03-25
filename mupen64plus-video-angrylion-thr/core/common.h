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
#define RGBA16_B(x)  (((x) & 0x3e) << 2)
#define RGBA16_G(x)  (((x) & 0x7c0) >> 3)
#define RGBA16_R(x)   (((x) >> 8) & 0xf8)

int32_t irand(void);
