/*
 *  Copyright 2015 Masatoshi Teruya. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a 
 *  copy of this software and associated documentation files (the "Software"), 
 *  to deal in the Software without restriction, including without limitation 
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 *  and/or sell copies of the Software, and to permit persons to whom the 
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 *  DEALINGS IN THE SOFTWARE.
 *
 *  parcel.h
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/01/15.
 *
 */

#ifndef ___PARCEL_H___
#define ___PARCEL_H___


#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

// MARK: errors

typedef enum {
    // no error
    PARCEL_OK           = 0,
    // ENOMEM: cannot allocate memory
    PARCEL_ENOMEM       = ENOMEM,
    // ENOBUFS: no memory block space available
    PARCEL_ENOBLKS      = ENOBUFS,
    // EILSEQ: illegal byte sequence
    PARCEL_EILSEQ       = EILSEQ,
    
    // packing
    // EDOM: argument out of domain
    PARCEL_EDOM         = EDOM,
    // ENOTSUP: operation not supported
    PARCEL_ENOTSUP      = ENOTSUP,
    
    // unpacking
    // ENODATA: no message available on memory block
    PARCEL_ENODATA = ENODATA
} par_error_t;


static inline char *par_strerror( par_error_t err )
{
    switch ( err ){
        // generic error
        case PARCEL_OK:
            return "no error";
        
        case PARCEL_ENOMEM:
            return "cannot allocate memory";
        
        case PARCEL_ENOBLKS:
            return "no memory block space available";
        
        case PARCEL_EILSEQ:
            return "illegal byte sequence";
        
        // packing error
        case PARCEL_EDOM:
            return "argument out of domain";
        
        case PARCEL_ENOTSUP:
            return "operation not supported";
        
        // unpacking error
        case PARCEL_ENODATA:
            return "no data available on memory block";
        
        default:
            return strerror( errno );
    }
}


// MARK: parcel data format

// attributes
#define PAR_A_NONE      0x0

// boolean
#define PAR_A_FALSE     PAR_A_NONE
#define PAR_A_TRUE      0x1

// length byte size
#define PAR_A_BIT8      PAR_A_NONE
#define PAR_A_BIT16     0x1
#define PAR_A_BIT32     0x2
#define PAR_A_BIT64     0x3

// convert bit value to byte size
#define _PAR_BIT2BYTE(f)    (1<<(f))

// array/map stream
#define PAR_A_STREAM    0x4

// endianness
#define PAR_A_LENDIAN   PAR_A_NONE
#define PAR_A_BENDIAN   0x1

// signedness
#define PAR_A_UNSIGN    PAR_A_NONE
#define PAR_A_SIGNED    0x1

// masks
#define PAR_NOMASK      PAR_A_NONE
#define PAR_MASK_ISA    0xF8
// mask attributes
#define PAR_MASK_ATTR   0x7
#define PAR_MASK_NUM    0x5     // endianess | signedness
#define PAR_MASK_ENDIAN 0x4
#define PAR_MASK_BIT    0x3
#define PAR_MASK_SIGN   0x1
#define PAR_MASK_INF    0x1
#define PAR_MASK_BOL    0x1
#define PAR_MASK_STR    PAR_MASK_ATTR


// type(5): 0-31
enum {
    //
    // 1 byte types
    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_NIL 00000|000
    // -----------------+----+
    // PAR_ISA_I0  00001|000
    // -----------------+----+
    // PAR_ISA_NAN 00010|000
    // -----------------+----+
    PAR_ISA_NIL = 0,    // nil
    PAR_ISA_I0  = 0x8,  // zero value
    PAR_ISA_NAN = 0x10, // NaN value

    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_BOL 00011|00X
    // -----------------+----+
    // X: 0 = PAR_A_FALSE, 1 = PAR_A_TRUE
    //
    PAR_ISA_BOL = 0x18, // boolean

    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_INF 00100|00X
    // -----------------+----+
    // X: 0 = PAR_A_UNSIGN, 1 = PAR_A_SIGNED
    //
    PAR_ISA_INF = 0x20, // infinity value

    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_EMP 00101|000
    // -----------------+----+
    // PAR_ISA_EOS 00110|000
    // -----------------+----+
    //
    PAR_ISA_EMP = 0x28, // empty table
    PAR_ISA_EOS = 0x30, // end-of-stream


    //
    // 1+[1-8] byte types
    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_I8  00111|X0Y
    // -----------------+----+
    // PAR_ISA_I16 01000|X0Y
    // -----------------+----+
    // PAR_ISA_I32 01001|X0Y
    // -----------------+----+
    // PAR_ISA_I64 01010|X0Y
    // -----------------+----+
    // PAR_ISA_F32 01011|X0Y
    // -----------------+----+
    // PAR_ISA_F64 01100|X0Y
    // -----------------+----+
    //
    // X: endianess
    // Y: signedness
    // ---------------+---------------+
    //               X|Y
    // ---------------+---------------+
    // PAR_A_LENDIAN 0|0 PAR_A_UNSIGN
    //               0|1 PAR_A_SIGNED
    // ---------------+---------------+
    // PAR_A_BENDIAN 1|0 PAR_A_UNSIGN
    //               1|1 PAR_A_SIGNED
    // ---------------+---------------+
    //
    // integer/floating-point
    // 0      1        ...      max 9
    // -------+---------------------+
    // type(1)| data(8-64 bit value)
    // -------+---------------------+
    //
    // integral number
    PAR_ISA_I8  = 0x38, // 8 bit
    PAR_ISA_I16 = 0x40, // 16 bit
    PAR_ISA_I32 = 0x48, // 32 bit
    PAR_ISA_I64 = 0x50, // 64 bit
    // floating-point number
    PAR_ISA_F32 = 0x58, // 32 bit
    PAR_ISA_F64 = 0x60, // 64 bit

    //
    // 1+[0-8] byte types
    //
    // -----------------+---+
    // type             |attr
    // -----------------+---+
    // PAR_ISA_ARR 01101|XYY
    // -----------------+---+
    // PAR_ISA_MAP 01110|XYY
    // -----------------+---+
    //
    // XYY is length of array/map
    // -+---------------+---------------------
    // X|YY             | size bit
    // -+---------------+---------------------
    // 0|00 PAR_A_BIT8  |  8 bit uint (1 byte)
    // -+---------------+---------------------
    // 0|01 PAR_A_BIT16 | 16 bit uint (2 byte)
    // -+---------------+---------------------
    // 0|10 PAR_A_BIT32 | 32 bit uint (4 byte)
    // -+---------------+---------------------
    // 0|11 PAR_A_BIT64 | 64 bit uint (8 byte)
    // -+---------------+---------------------
    // 1|00             | stream array/map
    // -+---------------+---------------------
    // 1|ZZ             | INVALID
    // -+---------------+---------------------
    // NOTE:
    //   if arrat/map stream; last item must be PAR_ISA_EOS type.
    //
    // fixed length array/map
    // 0      1   ...  max 9(YY byte)
    // -------+------------+---------------------
    // type(1)| length(YY) | serialized items...
    // -------+------------+---------------------
    //
    // stream array/map
    // 0      1
    // -------+---------------------+---------------+
    // type(1)| serialized items... | PAR_ISA_EOS(1)
    // -------+---------------------+---------------+
    //
    PAR_ISA_ARR = 0x68, // array
    PAR_ISA_MAP = 0x70, // map  

    //
    // 1+[1-8] byte types
    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_REF 01111|0XX
    // -----------------+----+
    //
    // XXX is cursor position of reference
    // ---------------+----------------------
    // XX             | size bit
    // ---------------+----------------------
    // 00 PAR_A_BIT8  |  8 bit uint (1 byte)
    // ---------------+----------------------
    // 01 PAR_A_BIT16 | 16 bit uint (2 byte)
    // ---------------+----------------------
    // 10 PAR_A_BIT32 | 32 bit uint (4 byte)
    // ---------------+----------------------
    // 11 PAR_A_BIT64 | 64 bit uint (8 byte)
    // ---------------+----------------------
    //
    // reference
    // 0      1         ...      max 9(XX byte)
    // -------+----------------------+
    // type(1)| previous position(XX)
    // -------+----------------------+
    //
    PAR_ISA_REF = 0x78, // reference

    //
    // 1+[1-8] byte types
    //
    // -----------------+----+
    // type             |attr
    // -----------------+----+
    // PAR_ISA_RAW 10000|XYY
    // -----------------+----+
    // PAR_ISA_STR 10001|XYY
    // -----------------+----+
    // X: 0 = PAR_A_LENDIAN, 1 = PAR_A_BENDIAN
    //
    // YY is size of raw/string, or cursor position of reference
    // ---------------+----------------------
    // YY             | size bit
    // ---------------+----------------------
    // 00 PAR_A_BIT8  |  8 bit uint (1 byte)
    // ---------------+----------------------
    // 01 PAR_A_BIT16 | 16 bit uint (2 byte)
    // ---------------+----------------------
    // 10 PAR_A_BIT32 | 32 bit uint (4 byte)
    // ---------------+----------------------
    // 11 PAR_A_BIT64 | 64 bit uint (8 byte)
    // ----------------+----------------------
    //
    // raw/string
    // 0      1     ...   max 9(YY byte)
    // -------+---------------+-----------------------------
    // type(1)| data size(YY) | data(uint YY bit length)...
    // -------+---------------+-----------------------------
    //
    PAR_ISA_RAW = 0x80, // raw data
    PAR_ISA_STR = 0x88  // string
};


// floating-point numbers
typedef float par_float32_t;
typedef double par_float64_t;

typedef union {
    uint_fast8_t isa;
} par_type_t;

typedef uint_fast64_t par_typelen_t;

// partype_t + uint_fast64_t
typedef union {
    uint_fast8_t size[9];
} par_typex_t;

// partype_t + uint_fast8_t
typedef union {
    uint_fast8_t size[2];
} par_type8_t;

// partype_t + uint_fast16_t
typedef union {
    uint_fast8_t size[3];
} par_type16_t;

// partype_t + uint_fast32_t
typedef union {
    uint_fast8_t size[5];
} par_type32_t;

// partype_t + uint_fast64_t
typedef par_typex_t par_type64_t;


// par_type[8-64]_t size
#define PAR_TYPE_SIZE       sizeof(par_type_t)
#define PAR_TYPEX_SIZE      sizeof(par_typex_t)
#define PAR_TYPE8_SIZE      sizeof(par_type8_t)
#define PAR_TYPE16_SIZE     sizeof(par_type16_t)
#define PAR_TYPE32_SIZE     sizeof(par_type32_t)
#define PAR_TYPE64_SIZE     sizeof(par_type64_t)



// MARK: endianness

// 0x0: little-endian, 0x4: big-endian
static inline uint8_t par_get_endian( void )
{
    int endian = 1;
    
    endian = !(*(char*)&endian);
    return (uint8_t)(endian << 2);
}


// MARK: memory block size

#define PAR_DEFAULT_BLK_SIZE    1024

static inline size_t _par_align_blksize( size_t blksize )
{
    if( !blksize ){
        return PAR_DEFAULT_BLK_SIZE;
    }
    else if( blksize < 16 ){
        return 16;
    }
    
    return blksize / 16 * 16;
}

#undef PAR_DEFAULT_BLK_SIZE


// check available block space
#define _PAR_CHECK_BLKSPC(blksize,cur,req) do { \
    if( (cur) >= (blksize) || ((blksize)-(cur)) < (req) ){ \
        errno = PARCEL_ENOBLKS; \
        return -1; \
    } \
}while(0)



// MARK: packing

typedef struct {
    uint8_t endian;
    size_t cur;
    size_t blksize;
    size_t nblkmax;
    size_t nblk;
    size_t bytes;
    void *mem;
} par_pack_t;


static inline int par_pack_init( par_pack_t *p, size_t blksize )
{
    blksize = _par_align_blksize( blksize );
    if( ( p->mem = malloc( blksize ) ) ){
        p->endian = par_get_endian();
        p->cur = 0;
        p->blksize = blksize;
        p->nblkmax = SIZE_MAX / blksize;
        p->nblk = 1;
        p->bytes = blksize;
        return 0;
    }
    
    return -1;
}



// for stream
typedef int (*par_reduce_t)( void *mem, size_t bytes, void *udata );

typedef struct {
    uint8_t endian;
    size_t cur;
    size_t blksize;
    void *mem;
    // stream
    par_reduce_t reducer;
    void *udata;
} par_spack_t;


static inline int par_spack_init( par_spack_t *p, size_t blksize, 
                                  par_reduce_t reducer, void *udata )
{
    blksize = _par_align_blksize( blksize );
    if( ( p->mem = malloc( blksize ) ) ){
        p->endian = par_get_endian();
        p->cur = 0;
        p->blksize = blksize;
        p->reducer = reducer;
        p->udata = udata;
        return 0;
    }
    
    return -1;
}


#define par_pack_dispose(p) do { \
    if( (p)->mem ){ \
        free( (p)->mem ); \
        (p)->mem = NULL; \
    } \
}while(0)


static void *_par_pack_increase( par_pack_t *p, size_t bytes )
{
    size_t remain = p->bytes - p->cur;
    
    if( remain < bytes )
    {
        size_t nblk = 0;
        
        // calculate number of block
        bytes -= remain;
        nblk += ( bytes / p->blksize ) + !!( bytes % p->blksize );
        if( nblk < ( p->nblkmax - p->nblk ) )
        {
            void *mem = NULL;
            
            bytes = p->blksize * ( p->nblk + nblk );
            // failed to allocate memory block
            if( !( mem = realloc( p->mem, bytes ) ) ){
                return NULL;
            }
            // update
            p->mem = mem;
            p->nblk += nblk;
            p->bytes = bytes;
        }
        else {
            errno = PARCEL_ENOMEM;
            return NULL;
        }
    }
    
    return p->mem;
}


static void *_par_pack_reduce( par_spack_t *p, size_t bytes )
{
    size_t remain = p->blksize - p->cur;
    
    if( remain < bytes )
    {
        // failed to reduce memory
        if( p->reducer( p->mem, p->cur, p->udata ) != 0 ){
            return NULL;
        }
        // rewind cursor
        p->cur = 0;
    }
    
    return p->mem;
}


static inline void *_par_pack_update_cur( par_pack_t *p, size_t bytes )
{
    void *mem = p->mem + p->cur;

    p->cur += bytes;
    
    return mem;
}


// allocate sizeof(t) and extra bytes
#define _PAR_PACK_SLICE( p, fn, l ) ({ \
    void *mem = fn( p, l ); \
    if( !mem ){ \
        return -1; \
    } \
    mem = (p)->mem + (p)->cur; \
    (p)->cur += l; \
    mem; \
})


static inline int _par_pack_type_set( par_type_t *pval, uint8_t type, 
                                      uint8_t attr, uint8_t domain )
{
    if( attr & ~domain ){
        errno = PARCEL_EDOM;
        return -1;
    }
    pval->isa = type | attr;
    
    return 0;
}


// MARK: packing one byte type

#define _PAR_PACK_TYPE( p, allocf, type, attr, domain ) ({ \
    par_type_t *_pval = _PAR_PACK_SLICE( p, allocf, PAR_TYPE_SIZE ); \
    _par_pack_type_set( _pval, type, attr, domain ); \
})


// nil
static inline int par_pack_nil( par_pack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_NIL, PAR_A_NONE, 
                           PAR_NOMASK );
}

// zero
static inline int par_pack_zero( par_pack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_I0, PAR_A_NONE, 
                           PAR_NOMASK );
}

// nan
static inline int par_pack_nan( par_pack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_NAN, PAR_A_NONE, 
                           PAR_NOMASK );
}

// bool
static inline int par_pack_bool( par_pack_t *p, uint8_t bol )
{
    return _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_BOL, bol, 
                           PAR_MASK_BOL );
}

// infinity
static inline int par_pack_inf( par_pack_t *p, uint8_t val )
{
    return _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_INF, val, 
                           PAR_MASK_INF );
}

// empty array/map
static inline int par_pack_empty( par_pack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_EMP, PAR_A_NONE, 
                           PAR_NOMASK );
}


// stream
// nil
static inline int par_spack_nil( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_NIL, PAR_A_NONE, 
                           PAR_NOMASK );
}

// zero
static inline int par_spack_zero( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_I0, PAR_A_NONE, 
                           PAR_NOMASK );
}

// nan
static inline int par_spack_nan( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_NAN, PAR_A_NONE, 
                           PAR_NOMASK );
}

// bool
static inline int par_spack_bool( par_spack_t *p, uint8_t bol )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_BOL, bol, 
                           PAR_MASK_BOL );
}

// infinity
static inline int par_spack_inf( par_spack_t *p, uint8_t val )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_INF, val, 
                           PAR_MASK_INF );
}

// empty array/map
static inline int par_spack_empty( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_EMP, PAR_A_NONE, 
                           PAR_NOMASK );
}

// array
static inline int par_spack_arr( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_ARR, PAR_A_STREAM, 
                           PAR_A_STREAM );
}

// map
static inline int par_spack_map( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_MAP, PAR_A_STREAM, 
                           PAR_A_STREAM );
}

// eos
static inline int par_spack_eos( par_spack_t *p )
{
    return _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_EOS, PAR_A_NONE, 
                           PAR_NOMASK );
}

// MARK: undef _PAR_PACK_TYPE
#undef _PAR_PACK_TYPE



// MARK: packing integeral number

#define _PAR_PACK_BITINT( p, allocf, bit, v, sign ) do { \
    par_type_t *pval = _PAR_PACK_SLICE( p, allocf, PAR_TYPE##bit##_SIZE ); \
    pval->isa = PAR_ISA_I##bit | p->endian | sign; \
    *((uint_fast##bit##_t*)(pval+PAR_TYPE_SIZE)) = (uint_fast##bit##_t)v; \
}while(0)


// positive integer
#define _PAR_PACK_UINT( p, allocf, num ) do { \
    if( num <= UINT8_MAX ){ \
        _PAR_PACK_BITINT( p, allocf, 8, num, PAR_A_UNSIGN ); \
    } \
    else if( num <= UINT16_MAX ){ \
        _PAR_PACK_BITINT( p, allocf, 16, num, PAR_A_UNSIGN ); \
    } \
    else if( num <= UINT32_MAX ){ \
        _PAR_PACK_BITINT( p, allocf, 32, num, PAR_A_UNSIGN ); \
    } \
    else { \
        _PAR_PACK_BITINT( p, allocf, 64, num, PAR_A_UNSIGN ); \
    } \
}while(0)


static inline int par_pack_uint( par_pack_t *p, uint_fast64_t num )
{
    _PAR_PACK_UINT( p, _par_pack_increase, num );
    return 0;
}

static inline int par_spack_uint( par_spack_t *p, uint_fast64_t num )
{
    _PAR_PACK_UINT( p, _par_pack_reduce, num );
    return 0;
}


// MARK: undef _PAR_PACK_UINT
#undef _PAR_PACK_UINT



// negative integer
#define _PAR_PACK_INT( p, allocf, num ) do { \
    if( num >= INT8_MIN ){ \
        _PAR_PACK_BITINT( p, allocf, 8, num, PAR_A_SIGNED ); \
    } \
    else if( num >= INT16_MIN ){ \
        _PAR_PACK_BITINT( p, allocf, 16, num, PAR_A_SIGNED ); \
    } \
    else if( num >= INT32_MIN ){ \
        _PAR_PACK_BITINT( p, allocf, 32, num, PAR_A_SIGNED ); \
    } \
    else { \
        _PAR_PACK_BITINT( p, allocf, 64, num, PAR_A_SIGNED ); \
    } \
}while(0)


static inline int par_pack_int( par_pack_t *p, int_fast64_t num )
{
    _PAR_PACK_INT( p, _par_pack_increase, num );
    return 0;
}

static inline int par_spack_int( par_spack_t *p, int_fast64_t num )
{
    _PAR_PACK_INT( p, _par_pack_reduce, num );
    return 0;
}


// MARK: undef _PAR_PACK_INT
#undef _PAR_PACK_INT
// MARK: undef _PAR_PACK_BITINT
#undef _PAR_PACK_BITINT



// MARK: packing floating-point number

#define _PAR_PACK_BITFLOAT( p, allocf, bit, v ) do { \
    uint_fast8_t sign = !!signbit( v ); \
    par_type_t *pval = _PAR_PACK_SLICE( p, allocf, PAR_TYPE##bit##_SIZE ); \
    pval->isa = PAR_ISA_F##bit | p->endian | (sign); \
    *((par_float##bit##_t*)(pval+PAR_TYPE_SIZE)) = (par_float##bit##_t)(v); \
}while(0)

static inline int par_pack_float32( par_pack_t *p, float num )
{
    _PAR_PACK_BITFLOAT( p, _par_pack_increase, 32, num );
    return 0;
}

static inline int par_pack_float64( par_pack_t *p, double num )
{
    _PAR_PACK_BITFLOAT( p, _par_pack_increase, 64, num );
    return 0;
}

static inline int par_spack_float32( par_spack_t *p, float num )
{
    _PAR_PACK_BITFLOAT( p, _par_pack_reduce, 32, num );
    return 0;
}

static inline int par_spack_float64( par_spack_t *p, double num )
{
    _PAR_PACK_BITFLOAT( p, _par_pack_reduce, 64, num );
    return 0;
}

// MARK: undef _PAR_PACK_BITFLOAT
#undef _PAR_PACK_BITFLOAT



// MARK: packing type with length value
#define _PAR_PACK_TYPE_WITH_NBITLEN( p, ptr, allocf, type, len, bit, ex ) do { \
    par_type_t *pval = _PAR_PACK_SLICE( p, allocf, PAR_TYPE##bit##_SIZE + ex ); \
    pval->isa = (type) | PAR_A_BIT##bit; \
    *(uint_fast##bit##_t*)(pval+PAR_TYPE_SIZE) = (uint_fast##bit##_t)len; \
    *(ptr) = (void*)(pval + PAR_TYPE##bit##_SIZE); \
}while(0)

#define _PAR_PACK_TYPE_WITH_LEN_EX( p, ptr, allocf, type, len, ex ) do { \
    /* 64bit */ \
    if( len & 0xFFFFFFFF00000000 ){ \
        _PAR_PACK_TYPE_WITH_NBITLEN( p, ptr, allocf, type, len, 64, ex ); \
    } \
    /* 32bit */ \
    else if( len & 0xFFFF0000 ){ \
        _PAR_PACK_TYPE_WITH_NBITLEN( p, ptr, allocf, type, len, 32, ex ); \
    } \
    /* 16bit */ \
    else if( len & 0xFF00 ){ \
        _PAR_PACK_TYPE_WITH_NBITLEN( p, ptr, allocf, type, len, 16, ex ); \
    } \
    /* 8bit */ \
    else { \
        _PAR_PACK_TYPE_WITH_NBITLEN( p, ptr, allocf, type, len, 8, ex ); \
    } \
}while(0)



#define _PAR_PACK_TYPE_WITH_LEN( p, allocf, type, len ) do { \
    void *_unused = NULL; \
    _PAR_PACK_TYPE_WITH_LEN_EX( p, &_unused, allocf, type, len, 0 ); \
}while(0)


// MARK: packing raw/string/ref

#define _PAR_PACK_BYTEA( p, allocf, type, val, len ) do { \
    void *dest = NULL; \
    /* allocate extra bytes space */ \
    _PAR_PACK_TYPE_WITH_LEN_EX( p, &dest, allocf, type, len, len ); \
    /* copy val */ \
    memcpy( dest, val, len ); \
}while(0)

static inline int par_pack_raw( par_pack_t *p, void *val, size_t len )
{
    _PAR_PACK_BYTEA( p, _par_pack_increase, PAR_ISA_RAW|p->endian, val, len );
    return 0;
}

static inline int par_pack_str( par_pack_t *p, void *val, size_t len )
{
    _PAR_PACK_BYTEA( p, _par_pack_increase, PAR_ISA_STR|p->endian, val, len );
    return 0;
}

// MARK: undef _PAR_PACK_BYTEA
#undef _PAR_PACK_BYTEA


#define _PAR_SPACK_BYTEA( p, val, len ) do { \
    size_t remain = p->blksize - p->cur; \
    /* copy to memory block if have space */ \
    if( remain >= len ){ \
COPY2BLOCK: \
        memcpy( p->mem + p->cur, val, len ); \
        p->cur += len; \
    } \
    else \
    { \
        /* copy remaining bytes */ \
        if( remain ){ \
            memcpy( p->mem + p->cur, val, remain ); \
            p->cur += remain; \
            val += remain; \
        } \
        /* reduce memory */ \
        if( p->reducer( p->mem, p->cur, p->udata ) == 0 ) \
        { \
            /* rewind cursor */ \
            p->cur = 0; \
            len -= remain; \
            /* copy to memory block if have space */ \
            if( len < p->blksize ){ \
                goto COPY2BLOCK; \
            } \
            /* reduce all */ \
            else if( p->reducer( val, len, p->udata ) != 0 ){ \
                return -1; \
            } \
        } \
    } \
}while(0)


static inline int par_spack_raw( par_spack_t *p, void *val, size_t len )
{
    _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_reduce, PAR_ISA_RAW, len );
    _PAR_SPACK_BYTEA( p, val, len );
    return 0;
}

static inline int par_spack_str( par_spack_t *p, void *val, size_t len )
{
    _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_reduce, PAR_ISA_STR|p->endian, len );
    _PAR_SPACK_BYTEA( p, val, len );
    return 0;
}

// MARK: undef _PAR_SPACK_BYTEA
#undef _PAR_SPACK_BYTEA


// array/map
static inline int par_pack_arr( par_pack_t *p, size_t len )
{
    _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_increase, PAR_ISA_ARR, len );
    return 0;
}

static inline int par_pack_map( par_pack_t *p, size_t len )
{
    _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_increase, PAR_ISA_MAP, len );
    return 0;
}


// MARK: undef _PAR_PACK_TYPE_WITH_LEN
#undef _PAR_PACK_TYPE_WITH_LEN
// MARK: undef _PAR_PACK_TYPE_WITH_LEN_EX
#undef _PAR_PACK_TYPE_WITH_LEN_EX
// MARK: undef _PAR_PACK_TYPE_WITH_NBITLEN
#undef _PAR_PACK_TYPE_WITH_NBITLEN


#define _PAR_PACK_TYPEXIDX( p, type, bit, idx ) do { \
    par_type_t *pval = NULL; \
    if( bit & ~(PAR_MASK_BIT) ){ \
        errno = PARCEL_EDOM; \
        return -1; \
    } \
    *idx = (p)->cur; \
    pval = _PAR_PACK_SLICE( p, _par_pack_increase, \
                            PAR_TYPE_SIZE + _PAR_BIT2BYTE( bit ) ); \
    pval->isa = type | bit; \
}while(0)


static inline int par_pack_arridx( par_pack_t *p, uint8_t bit, size_t *idx )
{
    _PAR_PACK_TYPEXIDX( p, PAR_ISA_ARR, bit, idx );
    return 0;
}

static inline int par_pack_mapidx( par_pack_t *p, uint8_t bit, size_t *idx )
{
    _PAR_PACK_TYPEXIDX( p, PAR_ISA_MAP, bit, idx );
    return 0;
}

// MARK: undef _PAR_PACK_TYPEXIDX
#undef _PAR_PACK_TYPEXIDX
// MARK: undef _PAR_PACK_SLICE
#undef _PAR_PACK_SLICE



static inline int par_pack_tbllen( par_pack_t *p, size_t idx, size_t len )
{
    // overflow
    if( idx > p->cur ){
        errno = PARCEL_EDOM;
        return -1;
    }
    else
    {
        par_type_t *pval = (par_type_t*)(p->mem + idx);
        
        switch( pval->isa & PAR_MASK_ISA )
        {
            case PAR_ISA_ARR:
            case PAR_ISA_MAP:
                // operation not supported
                if( ( pval->isa & PAR_MASK_ATTR ) > PAR_A_BIT64 ){
                    errno = PARCEL_ENOTSUP;
                    return -1;
                }
                else
                {
                    uint_fast8_t bit = pval->isa & PAR_MASK_BIT;
                    
                    // overflow
                    if( _PAR_BIT2BYTE( bit ) > ( p->cur - idx ) ){
                        errno = PARCEL_EDOM;
                        return -1;
                    }
                    
#define _PAR_SET_BITLEN(t,bit,len) \
    *(uint_fast##bit##_t*)(t+PAR_TYPE_SIZE) = (uint_fast##bit##_t)len;

                    switch( bit ){
                        case PAR_A_BIT8:
                            _PAR_SET_BITLEN( pval, 8, len );
                        break;
                        case PAR_A_BIT16:
                            _PAR_SET_BITLEN( pval, 16, len );
                        break;
                        case PAR_A_BIT32:
                            _PAR_SET_BITLEN( pval, 32, len );
                        break;
                        case PAR_A_BIT64:
                            _PAR_SET_BITLEN( pval, 64, len );
                        break;
                    }

#undef _PAR_SET_BITLEN

                    return 0;
                }
        }
        
        // illegal byte sequence
        errno = PARCEL_EILSEQ;
    }
    
    return -1;
}



// MARK: unpacking
// bin data
typedef struct {
    uint_fast8_t endian;
    size_t cur;
    size_t blksize;
    void *mem;
} par_unpack_t;


typedef struct {
    uint_fast8_t isa;
    uint_fast8_t endian;
    uint_fast8_t sign;
    uint_fast8_t attr;
    uint_fast64_t len;
    union {
        char *str;
        int_fast8_t i8;
        uint_fast8_t u8;
        int_fast16_t i16;
        uint_fast16_t u16;
        int_fast32_t i32;
        uint_fast32_t u32;
        int_fast64_t i64;
        uint_fast64_t u64;
        par_float32_t f32;
        par_float64_t f64;
    } val;
} par_extract_t;


static inline void par_unpack_init( par_unpack_t *p, void *mem, size_t blksize )
{
    static const int endian = 1;
    
    // 0: little-endian, 1: big-endian
    p->endian = !(*(char*)&endian);
    p->endian <<= 2;
    
    p->cur = 0;
    p->mem = mem;
    p->blksize = blksize;
}


// verify attribute
#define _PAR_VERIFY_ATTR(attr,mask) do { \
    if((attr) & ~(mask)){ \
        errno = PARCEL_EILSEQ; \
        return -1; \
    } \
}while(0)


// swap byteorder
#define _PAR_BSWAP8(v)

#define _PAR_BSWAP16(v) do { \
    v = ((((v)>>8)&0x00FF)|(((v)<<8)&0xFF00)); \
}while(0)

#define _PAR_BSWAP32(v) do { \
    v = ((((v)>>24)&0x000000FF)|(((v)>>8)&0x0000FF00) | \
         (((v)<<8)&0x00FF0000)|(((v)<<24)&0xFF000000)); \
}while(0)

#define _PAR_BSWAP64(v) do { \
    v = ((((v)>>56)&0x00000000000000FF)|(((v)>>40)&0x000000000000FF00) | \
         (((v)>>24)&0x0000000000FF0000)|(((v)>>8)&0x00000000FF000000) | \
         (((v)<<8)&0x000000FF00000000)|(((v)<<24)&0x0000FF0000000000) | \
         (((v)<<40)&0x00FF000000000000)|(((v)<<56)&0xFF00000000000000)); \
}while(0)


// extract nbit len value
#define _PAR_UNPACK_NBITLEN( p, type, ext, bit ) do { \
    (ext)->len = *(uint_fast##bit##_t*)( (type) + PAR_TYPE_SIZE ); \
    if( bit > 8 && (p)->endian != (ext)->endian ){ \
        _PAR_BSWAP##bit( (ext)->len ); \
    } \
    (p)->cur += PAR_TYPE##bit##_SIZE; \
}while(0)


#define _PAR_UNPACK_ARRMAP( p, type, ext ) do { \
    uint_fast8_t bit = (ext)->attr & PAR_MASK_BIT; \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, _PAR_BIT2BYTE( bit ) ); \
    switch( bit ){ \
        case PAR_A_BIT8: \
            _PAR_UNPACK_NBITLEN( p, type, ext, 8 ); \
        break; \
        case PAR_A_BIT16: \
            _PAR_UNPACK_NBITLEN( p, type, ext, 16 ); \
        break; \
        case PAR_A_BIT32: \
            _PAR_UNPACK_NBITLEN( p, type, ext, 32 ); \
        break; \
        case PAR_A_BIT64: \
            _PAR_UNPACK_NBITLEN( p, type, ext, 64 ); \
        break; \
    } \
}while(0)


// type: PAR_ISA_RAW, PAR_ISA_STR
// len: *(uint_fast[8-64]_t*)(mem + cur + PAR_TYPE_SIZE)
// val: mem + cur + PAR_TYPE[8-64]_SIZE
#define _PAR_UNPACK_NBIT_BYTEA( p, type, ext, bit ) do { \
    (ext)->val.str = (char*)( (type) + PAR_TYPE##bit##_SIZE ); \
    _PAR_UNPACK_NBITLEN( p, type, ext, bit ); \
    (p)->cur += (ext)->len; \
}while(0)


#define _PAR_UNPACK_BYTEA( p, type, ext ) do { \
    uint_fast8_t bit = (type)->isa & PAR_MASK_BIT; \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, _PAR_BIT2BYTE( bit ) ); \
    switch( bit ){ \
        case PAR_A_BIT8: \
            _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 8 ); \
        break; \
        case PAR_A_BIT16: \
            _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 16 ); \
        break; \
        case PAR_A_BIT32: \
            _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 32 ); \
        break; \
        case PAR_A_BIT64: \
            _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 64 ); \
        break; \
    } \
}while(0)


#define _PAR_UNPACK_BITINT( p, type, ext, bit ) do { \
    _PAR_VERIFY_ATTR( (ext)->attr, PAR_MASK_NUM ); \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, PAR_TYPE##bit##_SIZE ); \
    (ext)->val.u##bit = *(uint_fast##bit##_t*)( (type) + PAR_TYPE_SIZE ); \
    (p)->cur += PAR_TYPE##bit##_SIZE; \
    if( bit > 8 && (p)->endian != (ext)->endian ){ \
        _PAR_BSWAP##bit( (ext)->val.u##bit ); \
    } \
}while(0)


#define _PAR_UNPACK_BITFLOAT( p, type, ext, bit ) do { \
    _PAR_VERIFY_ATTR( (ext)->attr, PAR_MASK_NUM ); \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, PAR_TYPE##bit##_SIZE ); \
    (ext)->val.f##bit = *(par_float##bit##_t*)( (type) + PAR_TYPE_SIZE ); \
    (p)->cur += PAR_TYPE##bit##_SIZE; \
    if( (p)->endian != (ext)->endian ){ \
        _PAR_BSWAP##bit( (ext)->val.u##bit ); \
    } \
}while(0)


static inline int par_unpack( par_unpack_t *p, par_extract_t *ext )
{
    if( p->cur < p->blksize )
    {
        par_type_t *type = (par_type_t*)p->mem + p->cur;
        
        // init
        ext->isa = type->isa & PAR_MASK_ISA;
        ext->attr = type->isa & PAR_MASK_ATTR;
        ext->sign = ext->attr & PAR_MASK_SIGN;
        ext->endian = ext->attr & PAR_MASK_ENDIAN;
        ext->len = 0;
        
        switch( ext->isa ){
            // 1 byte types
            case PAR_ISA_NIL:
            case PAR_ISA_I0:
            case PAR_ISA_NAN:
            case PAR_ISA_EMP:
            case PAR_ISA_EOS:
                _PAR_VERIFY_ATTR( ext->attr, PAR_NOMASK );
                p->cur += PAR_TYPE_SIZE;
            break;
            
            case PAR_ISA_BOL:
            case PAR_ISA_INF:
                _PAR_VERIFY_ATTR( ext->attr, PAR_MASK_BOL );
                p->cur += PAR_TYPE_SIZE;
            break;
            
            // number
            // endian = 1:big-endian, 0:littel-endian
            // flag = 1:sign, 0:unsign
            case PAR_ISA_I8:
                _PAR_UNPACK_BITINT( p, type, ext, 8 );
            break;
            case PAR_ISA_I16:
                _PAR_UNPACK_BITINT( p, type, ext, 16 );
            break;
            case PAR_ISA_I32:
                _PAR_UNPACK_BITINT( p, type, ext, 32 );
            break;
            case PAR_ISA_I64:
                _PAR_UNPACK_BITINT( p, type, ext, 64 );
            break;
            case PAR_ISA_F32:
                _PAR_UNPACK_BITFLOAT( p, type, ext, 32 );
            break;
            case PAR_ISA_F64:
                _PAR_UNPACK_BITFLOAT( p, type, ext, 64 );
            break;
            
            case PAR_ISA_RAW:
                _PAR_VERIFY_ATTR( ext->attr, PAR_MASK_BIT );
            case PAR_ISA_STR:
                _PAR_UNPACK_BYTEA( p, type, ext );
            break;
            
            case PAR_ISA_ARR:
            case PAR_ISA_MAP:
                if( ext->attr & PAR_A_STREAM ){
                    _PAR_VERIFY_ATTR( ext->attr, PAR_A_STREAM );
                    p->cur += PAR_TYPE_SIZE;
                }
                else {
                    _PAR_UNPACK_ARRMAP( p, type, ext );
                }
            break;
            
            // illegal byte sequence
            default:
                errno = PARCEL_EILSEQ;
                return -1;
        }
        
        return 1;
    }
    
    // end-of-data
    // no message available on memory block
    errno = PARCEL_ENODATA;
    return 0;
}


// MARK: undef _PAR_BSWAP8
#undef _PAR_BSWAP8
// MARK: undef _PAR_BSWAP16
#undef _PAR_BSWAP16
// MARK: undef _PAR_BSWAP32
#undef _PAR_BSWAP32
// MARK: undef _PAR_BSWAP64
#undef _PAR_BSWAP64
// MARK: undef _PAR_UNPACK_NBITLEN
#undef _PAR_UNPACK_NBITLEN
// MARK: undef _PAR_UNPACK_ARRMAP
#undef _PAR_UNPACK_ARRMAP
// MARK: undef _PAR_UNPACK_NBIT_BYTEA
#undef _PAR_UNPACK_NBIT_BYTEA
// MARK: undef _PAR_UNPACK_BYTEA
#undef _PAR_UNPACK_BYTEA
// MARK: undef _PAR_UNPACK_BITINT
#undef _PAR_UNPACK_BITINT
// MARK: undef _PAR_UNPACK_BITFLOAT
#undef _PAR_UNPACK_BITFLOAT
// MARK: undef _PAR_VERIFY_ATTR
#undef _PAR_VERIFY_ATTR
// MARK: undef _PAR_BIT2BYTE
#undef _PAR_BIT2BYTE


#endif
