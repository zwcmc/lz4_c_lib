/*
*  xxHash - Fast Hash algorithm
*  Copyright (C) 2012-2016, Yann Collet
*
*  BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are
*  met:
*
*  * Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*  copyright notice, this list of conditions and the following disclaimer
*  in the documentation and/or other materials provided with the
*  distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*  You can contact the author at :
*  - LZ4_XXHash homepage: http://www.xxhash.com
*  - LZ4_XXHash source repository : https://github.com/Cyan4973/xxHash
*/


/* *************************************
*  Tuning parameters
***************************************/
/*!LZ4_XXH_FORCE_MEMORY_ACCESS :
 * By default, access to unaligned memory is controlled by `memcpy()`, which is safe and portable.
 * Unfortunately, on some target/compiler combinations, the generated assembly is sub-optimal.
 * The below switch allow to select different access method for improved performance.
 * Method 0 (default) : use `memcpy()`. Safe and portable.
 * Method 1 : `__packed` statement. It depends on compiler extension (ie, not portable).
 *            This method is safe if your compiler supports it, and *generally* as fast or faster than `memcpy`.
 * Method 2 : direct access. This method doesn't depend on compiler but violate C standard.
 *            It can generate buggy code on targets which do not support unaligned memory accesses.
 *            But in some circumstances, it's the only known way to get the most performance (ie GCC + ARMv6)
 * See http://stackoverflow.com/a/32095106/646947 for details.
 * Prefer these methods in priority order (0 > 1 > 2)
 */
#ifndef LZ4_XXH_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
#  if defined(__GNUC__) && ( defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) )
#    define LZ4_XXH_FORCE_MEMORY_ACCESS 2
#  elif (defined(__INTEL_COMPILER) && !defined(_WIN32)) || \
  (defined(__GNUC__) && ( defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) ))
#    define LZ4_XXH_FORCE_MEMORY_ACCESS 1
#  endif
#endif

/*!LZ4_XXH_ACCEPT_NULL_INPUT_POINTER :
 * If the input pointer is a null pointer, xxHash default behavior is to trigger a memory access error, since it is a bad pointer.
 * When this option is enabled, xxHash output for null input pointers will be the same as a null-length input.
 * By default, this option is disabled. To enable it, uncomment below define :
 */
/* #define LZ4_XXH_ACCEPT_NULL_INPUT_POINTER 1 */

/*!LZ4_XXH_FORCE_NATIVE_FORMAT :
 * By default, xxHash library provides endian-independent Hash values, based on little-endian convention.
 * Results are therefore identical for little-endian and big-endian CPU.
 * This comes at a performance cost for big-endian CPU, since some swapping is required to emulate little-endian format.
 * Should endian-independence be of no importance for your application, you may set the #define below to 1,
 * to improve speed for Big-endian CPU.
 * This option has no impact on Little_Endian CPU.
 */
#ifndef LZ4_XXH_FORCE_NATIVE_FORMAT   /* can be defined externally */
#  define LZ4_XXH_FORCE_NATIVE_FORMAT 0
#endif

/*!LZ4_XXH_FORCE_ALIGN_CHECK :
 * This is a minor performance trick, only useful with lots of very small keys.
 * It means : check for aligned/unaligned input.
 * The check costs one initial branch per hash; set to 0 when the input data
 * is guaranteed to be aligned.
 */
#ifndef LZ4_XXH_FORCE_ALIGN_CHECK /* can be defined externally */
#  if defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#    define LZ4_XXH_FORCE_ALIGN_CHECK 0
#  else
#    define LZ4_XXH_FORCE_ALIGN_CHECK 1
#  endif
#endif


/* *************************************
*  Includes & Memory related functions
***************************************/
/*! Modify the local functions below should you wish to use some other memory routines
*   for malloc(), free() */
#include <stdlib.h>
static void* LZ4_XXH_malloc(size_t s) { return malloc(s); }
static void  LZ4_XXH_free  (void* p)  { free(p); }
/*! and for memcpy() */
#include <string.h>
static void* LZ4_XXH_memcpy(void* dest, const void* src, size_t size) { return memcpy(dest,src,size); }

#define LZ4_XXH_STATIC_LINKING_ONLY
#include "lz4_xxhash.h"


/* *************************************
*  Compiler Specific Options
***************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  pragma warning(disable : 4127)      /* disable: C4127: conditional expression is constant */
#endif

#ifndef LZ4_XXH_FORCE_INLINE
#  ifdef _MSC_VER    /* Visual Studio */
#    define LZ4_XXH_FORCE_INLINE static __forceinline
#  else
#    if defined (__cplusplus) || defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#      ifdef __GNUC__
#        define LZ4_XXH_FORCE_INLINE static inline __attribute__((always_inline))
#      else
#        define LZ4_XXH_FORCE_INLINE static inline
#      endif
#    else
#      define LZ4_XXH_FORCE_INLINE static
#    endif /* __STDC_VERSION__ */
#  endif  /* _MSC_VER */
#endif /* LZ4_XXH_FORCE_INLINE */


/* *************************************
*  Basic Types
***************************************/
#ifndef MEM_MODULE
# if !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#   include <stdint.h>
    typedef uint8_t  BYTE;
    typedef uint16_t U16;
    typedef uint32_t U32;
    typedef  int32_t S32;
# else
    typedef unsigned char      BYTE;
    typedef unsigned short     U16;
    typedef unsigned int       U32;
    typedef   signed int       S32;
# endif
#endif

#if (defined(LZ4_XXH_FORCE_MEMORY_ACCESS) && (LZ4_XXH_FORCE_MEMORY_ACCESS==2))

/* Force direct memory access. Only works on CPU which support unaligned memory access in hardware */
static U32 LZ4_XXH_read32(const void* memPtr) { return *(const U32*) memPtr; }

#elif (defined(LZ4_XXH_FORCE_MEMORY_ACCESS) && (LZ4_XXH_FORCE_MEMORY_ACCESS==1))

/* __pack instructions are safer, but compiler specific, hence potentially problematic for some compilers */
/* currently only defined for gcc and icc */
typedef union { U32 u32; } __attribute__((packed)) unalign;
static U32 LZ4_XXH_read32(const void* ptr) { return ((const unalign*)ptr)->u32; }

#else

/* portable and safe solution. Generally efficient.
 * see : http://stackoverflow.com/a/32095106/646947
 */
static U32 LZ4_XXH_read32(const void* memPtr)
{
    U32 val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif   /* LZ4_XXH_FORCE_DIRECT_MEMORY_ACCESS */


/* ****************************************
*  Compiler-specific Functions and Macros
******************************************/
#define LZ4_XXH_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

/* Note : although _rotl exists for minGW (GCC under windows), performance seems poor */
#if defined(_MSC_VER)
#  define LZ4_XXH_rotl32(x,r) _rotl(x,r)
#  define LZ4_XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define LZ4_XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))
#  define LZ4_XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))
#endif

#if defined(_MSC_VER)     /* Visual Studio */
#  define LZ4_XXH_swap32 _byteswap_ulong
#elif LZ4_XXH_GCC_VERSION >= 403
#  define LZ4_XXH_swap32 __builtin_bswap32
#else
static U32 LZ4_XXH_swap32 (U32 x)
{
    return  ((x << 24) & 0xff000000 ) |
            ((x <<  8) & 0x00ff0000 ) |
            ((x >>  8) & 0x0000ff00 ) |
            ((x >> 24) & 0x000000ff );
}
#endif


/* *************************************
*  Architecture Macros
***************************************/
typedef enum { LZ4_XXH_bigEndian=0, LZ4_XXH_littleEndian=1 } LZ4_XXH_endianess;

/* LZ4_XXH_CPU_LITTLE_ENDIAN can be defined externally, for example on the compiler command line */
#ifndef LZ4_XXH_CPU_LITTLE_ENDIAN
    static const int g_one = 1;
#   define LZ4_XXH_CPU_LITTLE_ENDIAN   (*(const char*)(&g_one))
#endif


/* ***************************
*  Memory reads
*****************************/
typedef enum { LZ4_XXH_aligned, LZ4_XXH_unaligned } LZ4_XXH_alignment;

LZ4_XXH_FORCE_INLINE U32 LZ4_XXH_readLE32_align(const void* ptr, LZ4_XXH_endianess endian, LZ4_XXH_alignment align)
{
    if (align==LZ4_XXH_unaligned)
        return endian==LZ4_XXH_littleEndian ? LZ4_XXH_read32(ptr) : LZ4_XXH_swap32(LZ4_XXH_read32(ptr));
    else
        return endian==LZ4_XXH_littleEndian ? *(const U32*)ptr : LZ4_XXH_swap32(*(const U32*)ptr);
}

LZ4_XXH_FORCE_INLINE U32 LZ4_XXH_readLE32(const void* ptr, LZ4_XXH_endianess endian)
{
    return LZ4_XXH_readLE32_align(ptr, endian, LZ4_XXH_unaligned);
}

static U32 LZ4_XXH_readBE32(const void* ptr)
{
    return LZ4_XXH_CPU_LITTLE_ENDIAN ? LZ4_XXH_swap32(LZ4_XXH_read32(ptr)) : LZ4_XXH_read32(ptr);
}


/* *************************************
*  Macros
***************************************/
#define LZ4_XXH_STATIC_ASSERT(c)   { enum { LZ4_XXH_static_assert = 1/(int)(!!(c)) }; }    /* use only *after* variable declarations */
LZ4_XXH_PUBLIC_API unsigned LZ4_XXH_versionNumber (void) { return LZ4_XXH_VERSION_NUMBER; }


/* *******************************************************************
*  32-bits hash functions
*********************************************************************/
static const U32 PRIME32_1 = 2654435761U;
static const U32 PRIME32_2 = 2246822519U;
static const U32 PRIME32_3 = 3266489917U;
static const U32 PRIME32_4 =  668265263U;
static const U32 PRIME32_5 =  374761393U;

static U32 LZ4_XXH32_round(U32 seed, U32 input)
{
    seed += input * PRIME32_2;
    seed  = LZ4_XXH_rotl32(seed, 13);
    seed *= PRIME32_1;
    return seed;
}

LZ4_XXH_FORCE_INLINE U32 LZ4_XXH32_endian_align(const void* input, size_t len, U32 seed, LZ4_XXH_endianess endian, LZ4_XXH_alignment align)
{
    const BYTE* p = (const BYTE*)input;
    const BYTE* bEnd = p + len;
    U32 h32;
#define LZ4_XXH_get32bits(p) LZ4_XXH_readLE32_align(p, endian, align)

#ifdef LZ4_XXH_ACCEPT_NULL_INPUT_POINTER
    if (p==NULL) {
        len=0;
        bEnd=p=(const BYTE*)(size_t)16;
    }
#endif

    if (len>=16) {
        const BYTE* const limit = bEnd - 16;
        U32 v1 = seed + PRIME32_1 + PRIME32_2;
        U32 v2 = seed + PRIME32_2;
        U32 v3 = seed + 0;
        U32 v4 = seed - PRIME32_1;

        do {
            v1 = LZ4_XXH32_round(v1, LZ4_XXH_get32bits(p)); p+=4;
            v2 = LZ4_XXH32_round(v2, LZ4_XXH_get32bits(p)); p+=4;
            v3 = LZ4_XXH32_round(v3, LZ4_XXH_get32bits(p)); p+=4;
            v4 = LZ4_XXH32_round(v4, LZ4_XXH_get32bits(p)); p+=4;
        } while (p<=limit);

        h32 = LZ4_XXH_rotl32(v1, 1) + LZ4_XXH_rotl32(v2, 7) + LZ4_XXH_rotl32(v3, 12) + LZ4_XXH_rotl32(v4, 18);
    } else {
        h32  = seed + PRIME32_5;
    }

    h32 += (U32) len;

    while (p+4<=bEnd) {
        h32 += LZ4_XXH_get32bits(p) * PRIME32_3;
        h32  = LZ4_XXH_rotl32(h32, 17) * PRIME32_4 ;
        p+=4;
    }

    while (p<bEnd) {
        h32 += (*p) * PRIME32_5;
        h32 = LZ4_XXH_rotl32(h32, 11) * PRIME32_1 ;
        p++;
    }

    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}


LZ4_XXH_PUBLIC_API unsigned int LZ4_XXH32 (const void* input, size_t len, unsigned int seed)
{
#if 0
    /* Simple version, good for code maintenance, but unfortunately slow for small inputs */
    LZ4_XXH32_state_t state;
    LZ4_XXH32_reset(&state, seed);
    LZ4_XXH32_update(&state, input, len);
    return LZ4_XXH32_digest(&state);
#else
    LZ4_XXH_endianess endian_detected = (LZ4_XXH_endianess)LZ4_XXH_CPU_LITTLE_ENDIAN;

    if (LZ4_XXH_FORCE_ALIGN_CHECK) {
        if ((((size_t)input) & 3) == 0) {   /* Input is 4-bytes aligned, leverage the speed benefit */
            if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
                return LZ4_XXH32_endian_align(input, len, seed, LZ4_XXH_littleEndian, LZ4_XXH_aligned);
            else
                return LZ4_XXH32_endian_align(input, len, seed, LZ4_XXH_bigEndian, LZ4_XXH_aligned);
    }   }

    if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
        return LZ4_XXH32_endian_align(input, len, seed, LZ4_XXH_littleEndian, LZ4_XXH_unaligned);
    else
        return LZ4_XXH32_endian_align(input, len, seed, LZ4_XXH_bigEndian, LZ4_XXH_unaligned);
#endif
}



/*======   Hash streaming   ======*/

LZ4_XXH_PUBLIC_API LZ4_XXH32_state_t* LZ4_XXH32_createState(void)
{
    return (LZ4_XXH32_state_t*)LZ4_XXH_malloc(sizeof(LZ4_XXH32_state_t));
}
LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH32_freeState(LZ4_XXH32_state_t* statePtr)
{
    LZ4_XXH_free(statePtr);
    return LZ4_XXH_OK;
}

LZ4_XXH_PUBLIC_API void LZ4_XXH32_copyState(LZ4_XXH32_state_t* dstState, const LZ4_XXH32_state_t* srcState)
{
    memcpy(dstState, srcState, sizeof(*dstState));
}

LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH32_reset(LZ4_XXH32_state_t* statePtr, unsigned int seed)
{
    LZ4_XXH32_state_t state;   /* using a local state to memcpy() in order to avoid strict-aliasing warnings */
    memset(&state, 0, sizeof(state)-4);   /* do not write into reserved, for future removal */
    state.v1 = seed + PRIME32_1 + PRIME32_2;
    state.v2 = seed + PRIME32_2;
    state.v3 = seed + 0;
    state.v4 = seed - PRIME32_1;
    memcpy(statePtr, &state, sizeof(state));
    return LZ4_XXH_OK;
}


LZ4_XXH_FORCE_INLINE LZ4_XXH_errorcode LZ4_XXH32_update_endian (LZ4_XXH32_state_t* state, const void* input, size_t len, LZ4_XXH_endianess endian)
{
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;

#ifdef LZ4_XXH_ACCEPT_NULL_INPUT_POINTER
    if (input==NULL) return LZ4_XXH_ERROR;
#endif

    state->total_len_32 += (unsigned)len;
    state->large_len |= (len>=16) | (state->total_len_32>=16);

    if (state->memsize + len < 16)  {   /* fill in tmp buffer */
        LZ4_XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, len);
        state->memsize += (unsigned)len;
        return LZ4_XXH_OK;
    }

    if (state->memsize) {   /* some data left from previous update */
        LZ4_XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, 16-state->memsize);
        {   const U32* p32 = state->mem32;
            state->v1 = LZ4_XXH32_round(state->v1, LZ4_XXH_readLE32(p32, endian)); p32++;
            state->v2 = LZ4_XXH32_round(state->v2, LZ4_XXH_readLE32(p32, endian)); p32++;
            state->v3 = LZ4_XXH32_round(state->v3, LZ4_XXH_readLE32(p32, endian)); p32++;
            state->v4 = LZ4_XXH32_round(state->v4, LZ4_XXH_readLE32(p32, endian)); p32++;
        }
        p += 16-state->memsize;
        state->memsize = 0;
    }

    if (p <= bEnd-16) {
        const BYTE* const limit = bEnd - 16;
        U32 v1 = state->v1;
        U32 v2 = state->v2;
        U32 v3 = state->v3;
        U32 v4 = state->v4;

        do {
            v1 = LZ4_XXH32_round(v1, LZ4_XXH_readLE32(p, endian)); p+=4;
            v2 = LZ4_XXH32_round(v2, LZ4_XXH_readLE32(p, endian)); p+=4;
            v3 = LZ4_XXH32_round(v3, LZ4_XXH_readLE32(p, endian)); p+=4;
            v4 = LZ4_XXH32_round(v4, LZ4_XXH_readLE32(p, endian)); p+=4;
        } while (p<=limit);

        state->v1 = v1;
        state->v2 = v2;
        state->v3 = v3;
        state->v4 = v4;
    }

    if (p < bEnd) {
        LZ4_XXH_memcpy(state->mem32, p, (size_t)(bEnd-p));
        state->memsize = (unsigned)(bEnd-p);
    }

    return LZ4_XXH_OK;
}

LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH32_update (LZ4_XXH32_state_t* state_in, const void* input, size_t len)
{
    LZ4_XXH_endianess endian_detected = (LZ4_XXH_endianess)LZ4_XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
        return LZ4_XXH32_update_endian(state_in, input, len, LZ4_XXH_littleEndian);
    else
        return LZ4_XXH32_update_endian(state_in, input, len, LZ4_XXH_bigEndian);
}



LZ4_XXH_FORCE_INLINE U32 LZ4_XXH32_digest_endian (const LZ4_XXH32_state_t* state, LZ4_XXH_endianess endian)
{
    const BYTE * p = (const BYTE*)state->mem32;
    const BYTE* const bEnd = (const BYTE*)(state->mem32) + state->memsize;
    U32 h32;

    if (state->large_len) {
        h32 = LZ4_XXH_rotl32(state->v1, 1) + LZ4_XXH_rotl32(state->v2, 7) + LZ4_XXH_rotl32(state->v3, 12) + LZ4_XXH_rotl32(state->v4, 18);
    } else {
        h32 = state->v3 /* == seed */ + PRIME32_5;
    }

    h32 += state->total_len_32;

    while (p+4<=bEnd) {
        h32 += LZ4_XXH_readLE32(p, endian) * PRIME32_3;
        h32  = LZ4_XXH_rotl32(h32, 17) * PRIME32_4;
        p+=4;
    }

    while (p<bEnd) {
        h32 += (*p) * PRIME32_5;
        h32  = LZ4_XXH_rotl32(h32, 11) * PRIME32_1;
        p++;
    }

    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}


LZ4_XXH_PUBLIC_API unsigned int LZ4_XXH32_digest (const LZ4_XXH32_state_t* state_in)
{
    LZ4_XXH_endianess endian_detected = (LZ4_XXH_endianess)LZ4_XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
        return LZ4_XXH32_digest_endian(state_in, LZ4_XXH_littleEndian);
    else
        return LZ4_XXH32_digest_endian(state_in, LZ4_XXH_bigEndian);
}


/*======   Canonical representation   ======*/

/*! Default LZ4_XXH result types are basic unsigned 32 and 64 bits.
*   The canonical representation follows human-readable write convention, aka big-endian (large digits first).
*   These functions allow transformation of hash result into and from its canonical format.
*   This way, hash values can be written into a file or buffer, and remain comparable across different systems and programs.
*/

LZ4_XXH_PUBLIC_API void LZ4_XXH32_canonicalFromHash(LZ4_XXH32_canonical_t* dst, LZ4_XXH32_hash_t hash)
{
    LZ4_XXH_STATIC_ASSERT(sizeof(LZ4_XXH32_canonical_t) == sizeof(LZ4_XXH32_hash_t));
    if (LZ4_XXH_CPU_LITTLE_ENDIAN) hash = LZ4_XXH_swap32(hash);
    memcpy(dst, &hash, sizeof(*dst));
}

LZ4_XXH_PUBLIC_API LZ4_XXH32_hash_t LZ4_XXH32_hashFromCanonical(const LZ4_XXH32_canonical_t* src)
{
    return LZ4_XXH_readBE32(src);
}


#ifndef LZ4_XXH_NO_LONG_LONG

/* *******************************************************************
*  64-bits hash functions
*********************************************************************/

/*======   Memory access   ======*/

#ifndef MEM_MODULE
# define MEM_MODULE
# if !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
#   include <stdint.h>
    typedef uint64_t U64;
# else
    typedef unsigned long long U64;   /* if your compiler doesn't support unsigned long long, replace by another 64-bit type here. Note that xxhash.h will also need to be updated. */
# endif
#endif


#if (defined(LZ4_XXH_FORCE_MEMORY_ACCESS) && (LZ4_XXH_FORCE_MEMORY_ACCESS==2))

/* Force direct memory access. Only works on CPU which support unaligned memory access in hardware */
static U64 LZ4_XXH_read64(const void* memPtr) { return *(const U64*) memPtr; }

#elif (defined(LZ4_XXH_FORCE_MEMORY_ACCESS) && (LZ4_XXH_FORCE_MEMORY_ACCESS==1))

/* __pack instructions are safer, but compiler specific, hence potentially problematic for some compilers */
/* currently only defined for gcc and icc */
typedef union { U32 u32; U64 u64; } __attribute__((packed)) unalign64;
static U64 LZ4_XXH_read64(const void* ptr) { return ((const unalign64*)ptr)->u64; }

#else

/* portable and safe solution. Generally efficient.
 * see : http://stackoverflow.com/a/32095106/646947
 */

static U64 LZ4_XXH_read64(const void* memPtr)
{
    U64 val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif   /* LZ4_XXH_FORCE_DIRECT_MEMORY_ACCESS */

#if defined(_MSC_VER)     /* Visual Studio */
#  define LZ4_XXH_swap64 _byteswap_uint64
#elif LZ4_XXH_GCC_VERSION >= 403
#  define LZ4_XXH_swap64 __builtin_bswap64
#else
static U64 LZ4_XXH_swap64 (U64 x)
{
    return  ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8)  & 0x000000ff00000000ULL) |
            ((x >> 8)  & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);
}
#endif

LZ4_XXH_FORCE_INLINE U64 LZ4_XXH_readLE64_align(const void* ptr, LZ4_XXH_endianess endian, LZ4_XXH_alignment align)
{
    if (align==LZ4_XXH_unaligned)
        return endian==LZ4_XXH_littleEndian ? LZ4_XXH_read64(ptr) : LZ4_XXH_swap64(LZ4_XXH_read64(ptr));
    else
        return endian==LZ4_XXH_littleEndian ? *(const U64*)ptr : LZ4_XXH_swap64(*(const U64*)ptr);
}

LZ4_XXH_FORCE_INLINE U64 LZ4_XXH_readLE64(const void* ptr, LZ4_XXH_endianess endian)
{
    return LZ4_XXH_readLE64_align(ptr, endian, LZ4_XXH_unaligned);
}

static U64 LZ4_XXH_readBE64(const void* ptr)
{
    return LZ4_XXH_CPU_LITTLE_ENDIAN ? LZ4_XXH_swap64(LZ4_XXH_read64(ptr)) : LZ4_XXH_read64(ptr);
}


/*======   LZ4_XXH64   ======*/

static const U64 PRIME64_1 = 11400714785074694791ULL;
static const U64 PRIME64_2 = 14029467366897019727ULL;
static const U64 PRIME64_3 =  1609587929392839161ULL;
static const U64 PRIME64_4 =  9650029242287828579ULL;
static const U64 PRIME64_5 =  2870177450012600261ULL;

static U64 LZ4_XXH64_round(U64 acc, U64 input)
{
    acc += input * PRIME64_2;
    acc  = LZ4_XXH_rotl64(acc, 31);
    acc *= PRIME64_1;
    return acc;
}

static U64 LZ4_XXH64_mergeRound(U64 acc, U64 val)
{
    val  = LZ4_XXH64_round(0, val);
    acc ^= val;
    acc  = acc * PRIME64_1 + PRIME64_4;
    return acc;
}

LZ4_XXH_FORCE_INLINE U64 LZ4_XXH64_endian_align(const void* input, size_t len, U64 seed, LZ4_XXH_endianess endian, LZ4_XXH_alignment align)
{
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;
    U64 h64;
#define LZ4_XXH_get64bits(p) LZ4_XXH_readLE64_align(p, endian, align)

#ifdef LZ4_XXH_ACCEPT_NULL_INPUT_POINTER
    if (p==NULL) {
        len=0;
        bEnd=p=(const BYTE*)(size_t)32;
    }
#endif

    if (len>=32) {
        const BYTE* const limit = bEnd - 32;
        U64 v1 = seed + PRIME64_1 + PRIME64_2;
        U64 v2 = seed + PRIME64_2;
        U64 v3 = seed + 0;
        U64 v4 = seed - PRIME64_1;

        do {
            v1 = LZ4_XXH64_round(v1, LZ4_XXH_get64bits(p)); p+=8;
            v2 = LZ4_XXH64_round(v2, LZ4_XXH_get64bits(p)); p+=8;
            v3 = LZ4_XXH64_round(v3, LZ4_XXH_get64bits(p)); p+=8;
            v4 = LZ4_XXH64_round(v4, LZ4_XXH_get64bits(p)); p+=8;
        } while (p<=limit);

        h64 = LZ4_XXH_rotl64(v1, 1) + LZ4_XXH_rotl64(v2, 7) + LZ4_XXH_rotl64(v3, 12) + LZ4_XXH_rotl64(v4, 18);
        h64 = LZ4_XXH64_mergeRound(h64, v1);
        h64 = LZ4_XXH64_mergeRound(h64, v2);
        h64 = LZ4_XXH64_mergeRound(h64, v3);
        h64 = LZ4_XXH64_mergeRound(h64, v4);

    } else {
        h64  = seed + PRIME64_5;
    }

    h64 += (U64) len;

    while (p+8<=bEnd) {
        U64 const k1 = LZ4_XXH64_round(0, LZ4_XXH_get64bits(p));
        h64 ^= k1;
        h64  = LZ4_XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
        p+=8;
    }

    if (p+4<=bEnd) {
        h64 ^= (U64)(LZ4_XXH_get32bits(p)) * PRIME64_1;
        h64 = LZ4_XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p+=4;
    }

    while (p<bEnd) {
        h64 ^= (*p) * PRIME64_5;
        h64 = LZ4_XXH_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}


LZ4_XXH_PUBLIC_API unsigned long long LZ4_XXH64 (const void* input, size_t len, unsigned long long seed)
{
#if 0
    /* Simple version, good for code maintenance, but unfortunately slow for small inputs */
    LZ4_XXH64_state_t state;
    LZ4_XXH64_reset(&state, seed);
    LZ4_XXH64_update(&state, input, len);
    return LZ4_XXH64_digest(&state);
#else
    LZ4_XXH_endianess endian_detected = (LZ4_XXH_endianess)LZ4_XXH_CPU_LITTLE_ENDIAN;

    if (LZ4_XXH_FORCE_ALIGN_CHECK) {
        if ((((size_t)input) & 7)==0) {  /* Input is aligned, let's leverage the speed advantage */
            if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
                return LZ4_XXH64_endian_align(input, len, seed, LZ4_XXH_littleEndian, LZ4_XXH_aligned);
            else
                return LZ4_XXH64_endian_align(input, len, seed, LZ4_XXH_bigEndian, LZ4_XXH_aligned);
    }   }

    if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
        return LZ4_XXH64_endian_align(input, len, seed, LZ4_XXH_littleEndian, LZ4_XXH_unaligned);
    else
        return LZ4_XXH64_endian_align(input, len, seed, LZ4_XXH_bigEndian, LZ4_XXH_unaligned);
#endif
}

/*======   Hash Streaming   ======*/

LZ4_XXH_PUBLIC_API LZ4_XXH64_state_t* LZ4_XXH64_createState(void)
{
    return (LZ4_XXH64_state_t*)LZ4_XXH_malloc(sizeof(LZ4_XXH64_state_t));
}
LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH64_freeState(LZ4_XXH64_state_t* statePtr)
{
    LZ4_XXH_free(statePtr);
    return LZ4_XXH_OK;
}

LZ4_XXH_PUBLIC_API void LZ4_XXH64_copyState(LZ4_XXH64_state_t* dstState, const LZ4_XXH64_state_t* srcState)
{
    memcpy(dstState, srcState, sizeof(*dstState));
}

LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH64_reset(LZ4_XXH64_state_t* statePtr, unsigned long long seed)
{
    LZ4_XXH64_state_t state;   /* using a local state to memcpy() in order to avoid strict-aliasing warnings */
    memset(&state, 0, sizeof(state)-8);   /* do not write into reserved, for future removal */
    state.v1 = seed + PRIME64_1 + PRIME64_2;
    state.v2 = seed + PRIME64_2;
    state.v3 = seed + 0;
    state.v4 = seed - PRIME64_1;
    memcpy(statePtr, &state, sizeof(state));
    return LZ4_XXH_OK;
}

LZ4_XXH_FORCE_INLINE LZ4_XXH_errorcode LZ4_XXH64_update_endian (LZ4_XXH64_state_t* state, const void* input, size_t len, LZ4_XXH_endianess endian)
{
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;

#ifdef LZ4_XXH_ACCEPT_NULL_INPUT_POINTER
    if (input==NULL) return LZ4_XXH_ERROR;
#endif

    state->total_len += len;

    if (state->memsize + len < 32) {  /* fill in tmp buffer */
        LZ4_XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, len);
        state->memsize += (U32)len;
        return LZ4_XXH_OK;
    }

    if (state->memsize) {   /* tmp buffer is full */
        LZ4_XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, 32-state->memsize);
        state->v1 = LZ4_XXH64_round(state->v1, LZ4_XXH_readLE64(state->mem64+0, endian));
        state->v2 = LZ4_XXH64_round(state->v2, LZ4_XXH_readLE64(state->mem64+1, endian));
        state->v3 = LZ4_XXH64_round(state->v3, LZ4_XXH_readLE64(state->mem64+2, endian));
        state->v4 = LZ4_XXH64_round(state->v4, LZ4_XXH_readLE64(state->mem64+3, endian));
        p += 32-state->memsize;
        state->memsize = 0;
    }

    if (p+32 <= bEnd) {
        const BYTE* const limit = bEnd - 32;
        U64 v1 = state->v1;
        U64 v2 = state->v2;
        U64 v3 = state->v3;
        U64 v4 = state->v4;

        do {
            v1 = LZ4_XXH64_round(v1, LZ4_XXH_readLE64(p, endian)); p+=8;
            v2 = LZ4_XXH64_round(v2, LZ4_XXH_readLE64(p, endian)); p+=8;
            v3 = LZ4_XXH64_round(v3, LZ4_XXH_readLE64(p, endian)); p+=8;
            v4 = LZ4_XXH64_round(v4, LZ4_XXH_readLE64(p, endian)); p+=8;
        } while (p<=limit);

        state->v1 = v1;
        state->v2 = v2;
        state->v3 = v3;
        state->v4 = v4;
    }

    if (p < bEnd) {
        LZ4_XXH_memcpy(state->mem64, p, (size_t)(bEnd-p));
        state->memsize = (unsigned)(bEnd-p);
    }

    return LZ4_XXH_OK;
}

LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH64_update (LZ4_XXH64_state_t* state_in, const void* input, size_t len)
{
    LZ4_XXH_endianess endian_detected = (LZ4_XXH_endianess)LZ4_XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
        return LZ4_XXH64_update_endian(state_in, input, len, LZ4_XXH_littleEndian);
    else
        return LZ4_XXH64_update_endian(state_in, input, len, LZ4_XXH_bigEndian);
}

LZ4_XXH_FORCE_INLINE U64 LZ4_XXH64_digest_endian (const LZ4_XXH64_state_t* state, LZ4_XXH_endianess endian)
{
    const BYTE * p = (const BYTE*)state->mem64;
    const BYTE* const bEnd = (const BYTE*)state->mem64 + state->memsize;
    U64 h64;

    if (state->total_len >= 32) {
        U64 const v1 = state->v1;
        U64 const v2 = state->v2;
        U64 const v3 = state->v3;
        U64 const v4 = state->v4;

        h64 = LZ4_XXH_rotl64(v1, 1) + LZ4_XXH_rotl64(v2, 7) + LZ4_XXH_rotl64(v3, 12) + LZ4_XXH_rotl64(v4, 18);
        h64 = LZ4_XXH64_mergeRound(h64, v1);
        h64 = LZ4_XXH64_mergeRound(h64, v2);
        h64 = LZ4_XXH64_mergeRound(h64, v3);
        h64 = LZ4_XXH64_mergeRound(h64, v4);
    } else {
        h64  = state->v3 + PRIME64_5;
    }

    h64 += (U64) state->total_len;

    while (p+8<=bEnd) {
        U64 const k1 = LZ4_XXH64_round(0, LZ4_XXH_readLE64(p, endian));
        h64 ^= k1;
        h64  = LZ4_XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
        p+=8;
    }

    if (p+4<=bEnd) {
        h64 ^= (U64)(LZ4_XXH_readLE32(p, endian)) * PRIME64_1;
        h64  = LZ4_XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p+=4;
    }

    while (p<bEnd) {
        h64 ^= (*p) * PRIME64_5;
        h64  = LZ4_XXH_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}

LZ4_XXH_PUBLIC_API unsigned long long LZ4_XXH64_digest (const LZ4_XXH64_state_t* state_in)
{
    LZ4_XXH_endianess endian_detected = (LZ4_XXH_endianess)LZ4_XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==LZ4_XXH_littleEndian) || LZ4_XXH_FORCE_NATIVE_FORMAT)
        return LZ4_XXH64_digest_endian(state_in, LZ4_XXH_littleEndian);
    else
        return LZ4_XXH64_digest_endian(state_in, LZ4_XXH_bigEndian);
}


/*====== Canonical representation   ======*/

LZ4_XXH_PUBLIC_API void LZ4_XXH64_canonicalFromHash(LZ4_XXH64_canonical_t* dst, LZ4_XXH64_hash_t hash)
{
    LZ4_XXH_STATIC_ASSERT(sizeof(LZ4_XXH64_canonical_t) == sizeof(LZ4_XXH64_hash_t));
    if (LZ4_XXH_CPU_LITTLE_ENDIAN) hash = LZ4_XXH_swap64(hash);
    memcpy(dst, &hash, sizeof(*dst));
}

LZ4_XXH_PUBLIC_API LZ4_XXH64_hash_t LZ4_XXH64_hashFromCanonical(const LZ4_XXH64_canonical_t* src)
{
    return LZ4_XXH_readBE64(src);
}

#endif  /* LZ4_XXH_NO_LONG_LONG */
