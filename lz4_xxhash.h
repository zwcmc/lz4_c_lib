/*
   LZ4_XXHash - Extremely Fast Hash algorithm
   Header File
   Copyright (C) 2012-2016, Yann Collet.

   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - LZ4_XXHash source repository : https://github.com/Cyan4973/LZ4_XXHash
*/

/* Notice extracted from LZ4_XXHash homepage :

LZ4_XXHash is an extremely fast Hash algorithm, running at RAM speed limits.
It also successfully passes all tests from the SMHasher suite.

Comparison (single thread, Windows Seven 32 bits, using SMHasher on a Core 2 Duo @3GHz)

Name            Speed       Q.Score   Author
LZ4_XXHash          5.4 GB/s     10
CrapWow         3.2 GB/s      2       Andrew
MumurHash 3a    2.7 GB/s     10       Austin Appleby
SpookyHash      2.0 GB/s     10       Bob Jenkins
SBox            1.4 GB/s      9       Bret Mulvey
Lookup3         1.2 GB/s      9       Bob Jenkins
SuperFastHash   1.2 GB/s      1       Paul Hsieh
CityHash64      1.05 GB/s    10       Pike & Alakuijala
FNV             0.55 GB/s     5       Fowler, Noll, Vo
CRC32           0.43 GB/s     9
MD5-32          0.33 GB/s    10       Ronald L. Rivest
SHA1-32         0.28 GB/s    10

Q.Score is a measure of quality of the hash function.
It depends on successfully passing SMHasher test set.
10 is a perfect score.

A 64-bits version, named LZ4_XXH64, is available since r35.
It offers much better speed, but for 64-bits applications only.
Name     Speed on 64 bits    Speed on 32 bits
LZ4_XXH64       13.8 GB/s            1.9 GB/s
LZ4_XXH32        6.8 GB/s            6.0 GB/s
*/

#ifndef LZ4_XXHASH_H_5627135585666179
#define LZ4_XXHASH_H_5627135585666179 1

#if defined (__cplusplus)
extern "C" {
#endif


/* ****************************
*  Definitions
******************************/
#include <stddef.h>   /* size_t */
typedef enum { LZ4_XXH_OK=0, LZ4_XXH_ERROR } LZ4_XXH_errorcode;


/* ****************************
*  API modifier
******************************/
/** LZ4_XXH_PRIVATE_API
*   This is useful to include LZ4_XXHash functions in `static` mode
*   in order to inline them, and remove their symbol from the public list.
*   Methodology :
*     #define LZ4_XXH_PRIVATE_API
*     #include "LZ4_XXHash.h"
*   `LZ4_XXHash.c` is automatically included.
*   It's not useful to compile and link it as a separate module.
*/
#ifdef LZ4_XXH_PRIVATE_API
#  ifndef LZ4_XXH_STATIC_LINKING_ONLY
#    define LZ4_XXH_STATIC_LINKING_ONLY
#  endif
#  if defined(__GNUC__)
#    define LZ4_XXH_PUBLIC_API static __inline __attribute__((unused))
#  elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#    define LZ4_XXH_PUBLIC_API static inline
#  elif defined(_MSC_VER)
#    define LZ4_XXH_PUBLIC_API static __inline
#  else
#    define LZ4_XXH_PUBLIC_API static   /* this version may generate warnings for unused static functions; disable the relevant warning */
#  endif
#else
#  define LZ4_XXH_PUBLIC_API   /* do nothing */
#endif /* LZ4_XXH_PRIVATE_API */

/*!LZ4_XXH_NAMESPACE, aka Namespace Emulation :

If you want to include _and expose_ LZ4_XXHash functions from within your own library,
but also want to avoid symbol collisions with other libraries which may also include LZ4_XXHash,

you can use LZ4_XXH_NAMESPACE, to automatically prefix any public symbol from LZ4_XXHash library
with the value of LZ4_XXH_NAMESPACE (therefore, avoid NULL and numeric values).

Note that no change is required within the calling program as long as it includes `LZ4_XXHash.h` :
regular symbol name will be automatically translated by this header.
*/
#ifdef LZ4_XXH_NAMESPACE
#  define LZ4_XXH_CAT(A,B) A##B
#  define LZ4_XXH_NAME2(A,B) LZ4_XXH_CAT(A,B)
#  define LZ4_XXH_versionNumber LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH_versionNumber)
#  define LZ4_XXH32 LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32)
#  define LZ4_XXH32_createState LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_createState)
#  define LZ4_XXH32_freeState LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_freeState)
#  define LZ4_XXH32_reset LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_reset)
#  define LZ4_XXH32_update LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_update)
#  define LZ4_XXH32_digest LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_digest)
#  define LZ4_XXH32_copyState LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_copyState)
#  define LZ4_XXH32_canonicalFromHash LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_canonicalFromHash)
#  define LZ4_XXH32_hashFromCanonical LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH32_hashFromCanonical)
#  define LZ4_XXH64 LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64)
#  define LZ4_XXH64_createState LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_createState)
#  define LZ4_XXH64_freeState LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_freeState)
#  define LZ4_XXH64_reset LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_reset)
#  define LZ4_XXH64_update LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_update)
#  define LZ4_XXH64_digest LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_digest)
#  define LZ4_XXH64_copyState LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_copyState)
#  define LZ4_XXH64_canonicalFromHash LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_canonicalFromHash)
#  define LZ4_XXH64_hashFromCanonical LZ4_XXH_NAME2(LZ4_XXH_NAMESPACE, LZ4_XXH64_hashFromCanonical)
#endif


/* *************************************
*  Version
***************************************/
#define LZ4_XXH_VERSION_MAJOR    0
#define LZ4_XXH_VERSION_MINOR    6
#define LZ4_XXH_VERSION_RELEASE  2
#define LZ4_XXH_VERSION_NUMBER  (LZ4_XXH_VERSION_MAJOR *100*100 + LZ4_XXH_VERSION_MINOR *100 + LZ4_XXH_VERSION_RELEASE)
LZ4_XXH_PUBLIC_API unsigned LZ4_XXH_versionNumber (void);


/*-**********************************************************************
*  32-bits hash
************************************************************************/
typedef unsigned int       LZ4_XXH32_hash_t;

/*! LZ4_XXH32() :
    Calculate the 32-bits hash of sequence "length" bytes stored at memory address "input".
    The memory between input & input+length must be valid (allocated and read-accessible).
    "seed" can be used to alter the result predictably.
    Speed on Core 2 Duo @ 3 GHz (single thread, SMHasher benchmark) : 5.4 GB/s */
LZ4_XXH_PUBLIC_API LZ4_XXH32_hash_t LZ4_XXH32 (const void* input, size_t length, unsigned int seed);

/*======   Streaming   ======*/
typedef struct LZ4_XXH32_state_s LZ4_XXH32_state_t;   /* incomplete type */
LZ4_XXH_PUBLIC_API LZ4_XXH32_state_t* LZ4_XXH32_createState(void);
LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode  LZ4_XXH32_freeState(LZ4_XXH32_state_t* statePtr);
LZ4_XXH_PUBLIC_API void LZ4_XXH32_copyState(LZ4_XXH32_state_t* dst_state, const LZ4_XXH32_state_t* src_state);

LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH32_reset  (LZ4_XXH32_state_t* statePtr, unsigned int seed);
LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH32_update (LZ4_XXH32_state_t* statePtr, const void* input, size_t length);
LZ4_XXH_PUBLIC_API LZ4_XXH32_hash_t  LZ4_XXH32_digest (const LZ4_XXH32_state_t* statePtr);

/*
These functions generate the LZ4_XXHash of an input provided in multiple segments.
Note that, for small input, they are slower than single-call functions, due to state management.
For small input, prefer `LZ4_XXH32()` and `LZ4_XXH64()` .

LZ4_XXH state must first be allocated, using LZ4_XXH*_createState() .

Start a new hash by initializing state with a seed, using LZ4_XXH*_reset().

Then, feed the hash state by calling LZ4_XXH*_update() as many times as necessary.
Obviously, input must be allocated and read accessible.
The function returns an error code, with 0 meaning OK, and any other value meaning there is an error.

Finally, a hash value can be produced anytime, by using LZ4_XXH*_digest().
This function returns the nn-bits hash as an int or long long.

It's still possible to continue inserting input into the hash state after a digest,
and generate some new hashes later on, by calling again LZ4_XXH*_digest().

When done, free LZ4_XXH state space if it was allocated dynamically.
*/

/*======   Canonical representation   ======*/

typedef struct { unsigned char digest[4]; } LZ4_XXH32_canonical_t;
LZ4_XXH_PUBLIC_API void LZ4_XXH32_canonicalFromHash(LZ4_XXH32_canonical_t* dst, LZ4_XXH32_hash_t hash);
LZ4_XXH_PUBLIC_API LZ4_XXH32_hash_t LZ4_XXH32_hashFromCanonical(const LZ4_XXH32_canonical_t* src);

/* Default result type for LZ4_XXH functions are primitive unsigned 32 and 64 bits.
*  The canonical representation uses human-readable write convention, aka big-endian (large digits first).
*  These functions allow transformation of hash result into and from its canonical format.
*  This way, hash values can be written into a file / memory, and remain comparable on different systems and programs.
*/


#ifndef LZ4_XXH_NO_LONG_LONG
/*-**********************************************************************
*  64-bits hash
************************************************************************/
typedef unsigned long long LZ4_XXH64_hash_t;

/*! LZ4_XXH64() :
    Calculate the 64-bits hash of sequence of length "len" stored at memory address "input".
    "seed" can be used to alter the result predictably.
    This function runs faster on 64-bits systems, but slower on 32-bits systems (see benchmark).
*/
LZ4_XXH_PUBLIC_API LZ4_XXH64_hash_t LZ4_XXH64 (const void* input, size_t length, unsigned long long seed);

/*======   Streaming   ======*/
typedef struct LZ4_XXH64_state_s LZ4_XXH64_state_t;   /* incomplete type */
LZ4_XXH_PUBLIC_API LZ4_XXH64_state_t* LZ4_XXH64_createState(void);
LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode  LZ4_XXH64_freeState(LZ4_XXH64_state_t* statePtr);
LZ4_XXH_PUBLIC_API void LZ4_XXH64_copyState(LZ4_XXH64_state_t* dst_state, const LZ4_XXH64_state_t* src_state);

LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH64_reset  (LZ4_XXH64_state_t* statePtr, unsigned long long seed);
LZ4_XXH_PUBLIC_API LZ4_XXH_errorcode LZ4_XXH64_update (LZ4_XXH64_state_t* statePtr, const void* input, size_t length);
LZ4_XXH_PUBLIC_API LZ4_XXH64_hash_t  LZ4_XXH64_digest (const LZ4_XXH64_state_t* statePtr);

/*======   Canonical representation   ======*/
typedef struct { unsigned char digest[8]; } LZ4_XXH64_canonical_t;
LZ4_XXH_PUBLIC_API void LZ4_XXH64_canonicalFromHash(LZ4_XXH64_canonical_t* dst, LZ4_XXH64_hash_t hash);
LZ4_XXH_PUBLIC_API LZ4_XXH64_hash_t LZ4_XXH64_hashFromCanonical(const LZ4_XXH64_canonical_t* src);
#endif  /* LZ4_XXH_NO_LONG_LONG */


#ifdef LZ4_XXH_STATIC_LINKING_ONLY

/* ================================================================================================
   This section contains definitions which are not guaranteed to remain stable.
   They may change in future versions, becoming incompatible with a different version of the library.
   They shall only be used with static linking.
   Never use these definitions in association with dynamic linking !
=================================================================================================== */

/* These definitions are only meant to allow allocation of LZ4_XXH state
   statically, on stack, or in a struct for example.
   Do not use members directly. */

   struct LZ4_XXH32_state_s {
       unsigned total_len_32;
       unsigned large_len;
       unsigned v1;
       unsigned v2;
       unsigned v3;
       unsigned v4;
       unsigned mem32[4];   /* buffer defined as U32 for alignment */
       unsigned memsize;
       unsigned reserved;   /* never read nor write, will be removed in a future version */
   };   /* typedef'd to LZ4_XXH32_state_t */

#ifndef LZ4_XXH_NO_LONG_LONG
   struct LZ4_XXH64_state_s {
       unsigned long long total_len;
       unsigned long long v1;
       unsigned long long v2;
       unsigned long long v3;
       unsigned long long v4;
       unsigned long long mem64[4];   /* buffer defined as U64 for alignment */
       unsigned memsize;
       unsigned reserved[2];          /* never read nor write, will be removed in a future version */
   };   /* typedef'd to LZ4_XXH64_state_t */
#endif

#  ifdef LZ4_XXH_PRIVATE_API
#    include "LZ4_XXHash.c"   /* include LZ4_XXHash function bodies as `static`, for inlining */
#  endif

#endif /* LZ4_XXH_STATIC_LINKING_ONLY */


#if defined (__cplusplus)
}
#endif

#endif /* LZ4_XXHASH_H_5627135585666179 */
