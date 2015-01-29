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
#include <float.h>
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


// MARK: parcel data format

enum {
    // 
    // 0x00-7F: 6 bit integer
    // -----------+----------+
    // type       |attr
    // -----------+--+------+
    // PAR_ISA_I7 |0X|ZZZZZZ
    // -----------+--+------+
    // 0          |00|000000
    // -----------+--+------+
    //  1 to  63  |00|XXXXXX
    // -----------+--+------+
    // -1 to -63  |01|XXXXXX
    // -----------+--+------+
    //
#define PAR_INT6_MAX   63
#define PAR_INT6_MIN   -63
    PAR_ISA_S6       = 0x00, // positive small integer
    PAR_ISA_S6_TAIL  = 0x3F,
    PAR_ISA_S6N      = 0x40, // negative small integer
    PAR_ISA_S6N_TAIL = 0x7F,
    
    //
    // 0x80-9F: integer, raw, string, reference, array, map
    // --------------------+---+
    // type                |attr
    // --------------------+---+
    // PAR_ISA_U+X   100000|XX
    // --------------------+---+
    // PAR_ISA_S+X   100001|XX
    // --------------------+---+
    // PAR_ISA_RAW+X 100010|XX    // bytes size
    // --------------------+---+
    // PAR_ISA_STR+X 100011|XX
    // --------------------+---+
    // PAR_ISA_REF+X 100100|XX    // cursor position byte
    // --------------------+---+
    // PAR_ISA_ARR+X 100101|XX    // item length byte
    // --------------------+---+
    // PAR_ISA_MAP+X 100110|XX
    // --------------------+---+
    // PAR_ISA_SET+X 100111|XX
    // --------------------+---+
    // XX: bitwidth
    // --------------------+---+
    //                     |XX
    // --------------------+---+
    //  8 bit uint(1 byte) |00
    // --------------------+---+
    // 16 bit uint(2 byte) |01
    // --------------------+---+
    // 32 bit uint(4 byte) |10
    // --------------------+---+
    // 64 bit uint(8 byte) |11
    // --------------------+---+
    //
    // ref
    // 0      1  ... max 9(YY byte)
    // -------+-----------------------------+
    // type(1)| position(uint YY bit value) |
    // -------+-----------------------------+
    //
    // raw/string
    // 0      1  ... max 9(YY byte)
    // -------+----------+-----------------------------
    // type(1)| size(YY) | data(uint YY bit length)...
    // -------+----------+-----------------------------
    //
    // array
    // 0      1  ... max 9(YY byte)
    // -------+----------+----------------------
    // type(1)| size(YY) | serialized val | ...
    // -------+----------+----------------------
    //
    // map
    // 0      1  ... max 9(YY byte)
    // -------+----------+---------------------------------------
    // type(1)| size(YY) | serialized key | serialized val | ...
    // -------+----------+---------------------------------------
    //
    // set
    // 0      1  ... max 9(YY byte)
    // -------+----------+----------------------
    // type(1)| size(YY) | serialized val | ...
    // -------+----------+----------------------
    //
    // 3 bit right shift
    //
    // uint
    PAR_ISA_U8   = 0x80, 
    PAR_ISA_U16,
    PAR_ISA_U32,
    PAR_ISA_U64,
    // int
    PAR_ISA_S8,
    PAR_ISA_S16,
    PAR_ISA_S32,
    PAR_ISA_S64,
    // raw data
    PAR_ISA_RAW8,
    PAR_ISA_RAW16,
    PAR_ISA_RAW32,
    PAR_ISA_RAW64,
    // string
    PAR_ISA_STR8,
    PAR_ISA_STR16,
    PAR_ISA_STR32,
    PAR_ISA_STR64,
    // reference
    PAR_ISA_REF8,
    PAR_ISA_REF16,
    PAR_ISA_REF32,
    PAR_ISA_REF64,
    // array
    PAR_ISA_ARR8,
    PAR_ISA_ARR16,
    PAR_ISA_ARR32,
    PAR_ISA_ARR64,
    // map
    PAR_ISA_MAP8,
    PAR_ISA_MAP16,
    PAR_ISA_MAP32,
    PAR_ISA_MAP64,
    // set
    PAR_ISA_SET8,
    PAR_ISA_SET16,
    PAR_ISA_SET32,
    PAR_ISA_SET64,
    
    //
    // 0xA0
    // ----------------------+
    // type
    // ----------------------+
    // PAR_ISA_NAN  1010 0000
    // ----------------------+
    //
    PAR_ISA_NAN,

    //
    // float: 0xA1-A3
    // ------------------+---+
    // type              |attr
    // ------------------+---+
    // PAR_ISA_FLT 101000|XX
    // ------------------+---
    // XX: bitwidth
    // --------------------+---+
    //                     |XX
    // --------------------+---+
    // 16 bit uint(2 byte) |01
    // --------------------+---+
    // 32 bit uint(4 byte) |10
    // --------------------+---+
    // 64 bit uint(8 byte) |11
    // --------------------+---+
    //
    PAR_ISA_F16,
    PAR_ISA_F32,
    PAR_ISA_F64,
    
    //
    // 0xA4-A8
    // ----------------------+
    // type
    // ----------------------+
    // PAR_ISA_NINF 1010 0100
    // ----------------------+
    // PAR_ISA_PINF 1010 0101
    // ----------------------+
    // PAR_ISA_TRUE 1010 0110
    // ----------------------+
    // PAR_ISA_FLSE 1010 0111
    // ----------------------+
    // PAR_ISA_NIL  1010 1000
    // ----------------------+
    //
    PAR_ISA_NINF,   // negative infinity
    PAR_ISA_PINF,   // positive infinity
    PAR_ISA_TRUE,   // true
    PAR_ISA_FALSE,  // false
    PAR_ISA_NIL,    // nil
    
    //
    // 0xA9
    // ----------------------+
    // type
    // ----------------------+
    // PAR_ISA_IDX  1010 1001
    // ----------------------+
    // for non-consecutive array
    //
    //     N                                                              M+1
    // ----+----------------+-----------------------+----------------+-----+
    // ... | PAR_ISA_IDX(1) | serialized idx(M val) | serialized val | ... 
    // ----+----------------+-----------------------+----------------+-----+
    // NOTE: the serialized-idx value(M) must be type of unsigned integer and 
    //       larger than number of N-objects.
    //       array index number(N) of unpacking process must be initialized by 
    //       serialized index value(M).
    //
    PAR_ISA_IDX,    // non-consecutive array index
    
    //
    // 0xAA-AD
    // ----------------------+
    // type
    // ----------------------+
    // PAR_ISA_EOS  1010 1010
    // ----------------------+
    // PAR_ISA_SARR 1010 1011
    // ----------------------+
    // PAR_ISA_SMAP 1010 1100
    // ----------------------+
    // PAR_ISA_SSET 1010 1101
    // ----------------------+
    // NOTE: last item must be PAR_ISA_EOS type.
    //
    // array
    // 0      1
    // -------+----------------+-----+----------------
    // type(1)| serialized val | ... | PAR_ISA_EOS(1)
    // -------+----------------+-----+----------------
    //
    // map
    // 0      1
    // -------+----------------+----------------+-----+----------------
    // type(1)| serialized key | serialized val | ... | PAR_ISA_EOS(1)
    // -------+----------------+----------------+-----+----------------
    //
    // set
    // 0      1
    // -------+----------------+-----+----------------
    // type(1)| serialized val | ... | PAR_ISA_EOS(1)
    // -------+----------------+-----+----------------
    //
    PAR_ISA_EOS,    // end-of-stream
    PAR_ISA_SARR,   // stream array
    PAR_ISA_SMAP,   // stream map
    PAR_ISA_SSET,   // stream set
    
    //
    // UNUSED: 0xAE-BF
    // ----------------------+
    // type
    // ----------------------+
    // ............ 1010 1110
    // ----------------------+
    // ............ 1010 1111
    // ----------------------+
    // ............ 1011 0000
    // ----------------------+
    // ............ 1011 0001
    // ----------------------+
    // ............ 1011 0010
    // ----------------------+
    //
    
    //
    // 0xC0-DF
    // ----------------------+
    // PAR_ISA_STR5 110|XXXXX
    // ----------------------+
    // 5 bit length string
    //
    // 0             1
    // --------------+-----------------------
    // type 110XXXXX | data(5 bit length)...
    // --------------+-----------------------
    //
    PAR_ISA_STR5      = 0xC0,
    PAR_ISA_STR5_TAIL = 0xDF,
    
    //
    // 0xE0-EF, 0xF0-FF
    // ----------------------+
    // PAR_ISA_ARR5 1110|XXXX
    // ----------------------+
    // PAR_ISA_MAP5 1111|XXXX
    // ----------------------+
    // 4 bit length array/map
    //
    PAR_ISA_ARR4      = 0xE0,  // fixed array
    PAR_ISA_ARR4_TAIL = 0xEF,
    PAR_ISA_MAP4      = 0xF0,  // fixed map
    PAR_ISA_MAP4_TAIL = 0xFF
};


// floating-point numbers
typedef float par_float32_t;
typedef double par_float64_t;

typedef union {
    uint_fast8_t isa;
} par_type_t;

typedef uint_fast64_t par_typelen_t;

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
typedef union {
    uint_fast8_t size[9];
} par_type64_t;

// par_type[8-64]_t size
#define PAR_TYPE_SIZE       sizeof(par_type_t)
#define PAR_TYPE8_SIZE      sizeof(par_type8_t)
#define PAR_TYPE16_SIZE     sizeof(par_type16_t)
#define PAR_TYPE32_SIZE     sizeof(par_type32_t)
#define PAR_TYPE64_SIZE     sizeof(par_type64_t)


// MARK: endianness

// 1: little-endian, 0: big-endian
static inline uint8_t par_get_endian( void )
{
    int endian = 1;
    return (*(uint8_t*)&endian);
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
        return PARCEL_OK;
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
        return PARCEL_OK;
    }
    
    return -1;
}


#define par_pack_dispose( p ) do { \
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


static inline int par_pack_merge( par_pack_t *pdest, par_pack_t *psrc )
{
    void *mem = _par_pack_increase( pdest, psrc->cur );
    
    if( mem ){
        memcpy( pdest->mem + pdest->cur, psrc->mem, psrc->cur );
        pdest->cur += psrc->cur;
        return PARCEL_OK;
    }
    
    return -1;
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


// allocate sizeof(t) and extra bytes
#define _PAR_PACK_SLICE( p, fn, l ) ({ \
    void *_mem = fn( p, l ); \
    if( !_mem ){ \
        return -1; \
    } \
    _mem = (p)->mem + (p)->cur; \
    (p)->cur += l; \
    _mem; \
})


static inline int _par_pack_type_set( par_type_t *pval, uint8_t type, 
                                      uint8_t attr, uint8_t domain )
{
    if( attr & ~domain ){
        errno = PARCEL_EDOM;
        return -1;
    }
    pval->isa = type | attr;
    
    return PARCEL_OK;
}

#define _PAR_PACK_TYPE_EX( p, allocf, type, ex, ptr ) do { \
    par_type_t *_pval = _PAR_PACK_SLICE( p, allocf, PAR_TYPE_SIZE + (ex) ); \
    _pval->isa = (type); \
    *(ptr) = (void*)(_pval + PAR_TYPE_SIZE); \
}while(0)


// MARK: packing one byte type
#define _PAR_PACK_TYPE( p, allocf, type ) do { \
    void *_unused = NULL; \
    _PAR_PACK_TYPE_EX( p, allocf, type, 0, &_unused ); \
}while(0)


// nil
static inline int par_pack_nil( par_pack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_NIL );
    return PARCEL_OK;
}

static inline int par_spack_nil( par_spack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_NIL );
    return PARCEL_OK;
}


// zero
static inline int par_pack_zero( par_pack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, 0 );
    return PARCEL_OK;
}
static inline int par_spack_zero( par_spack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, 0 );
    return PARCEL_OK;
}


// nan
static inline int par_pack_nan( par_pack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_NAN );
    return PARCEL_OK;
}

static inline int par_spack_nan( par_spack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_NAN );
    return PARCEL_OK;
}

// stream array
static inline int par_pack_sarray( par_pack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_SARR );
    return PARCEL_OK;
}

static inline int par_spack_sarray( par_spack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_SARR );
    return PARCEL_OK;
}

// stream map
static inline int par_pack_smap( par_pack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_SMAP );
    return PARCEL_OK;
}

static inline int par_spack_smap( par_spack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_SMAP );
    return PARCEL_OK;
}


// eos
static inline int par_pack_eos( par_pack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_EOS );
    return PARCEL_OK;
}

static inline int par_spack_eos( par_spack_t *p )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_EOS );
    return PARCEL_OK;
}


// boolean
static inline int par_pack_bool( par_pack_t *p, uint8_t bol )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_TRUE + !bol );
    return PARCEL_OK;
}

static inline int par_spack_bool( par_spack_t *p, uint8_t bol )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_TRUE + !bol );
    return PARCEL_OK;
}


// infinity
static inline int par_pack_inf( par_pack_t *p, double num )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_NINF + !signbit( num ) );
    return PARCEL_OK;
}

static inline int par_spack_inf( par_spack_t *p, double num )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_NINF + !signbit( num ) );
    return PARCEL_OK;
}


// MARK: packing integeral number
#define _PAR_PACK_NBIT_VAL_EX( p, ptr, allocf, type, bit, v, ex ) do { \
    par_type_t *_pval = _PAR_PACK_SLICE(p, allocf, PAR_TYPE##bit##_SIZE+(ex)); \
    void *_val = (void*)(_pval+PAR_TYPE_SIZE); \
    _pval->isa = type##bit; \
    memcpy( _val, (void*)&(v), bit >> 3 ); \
    if( (p)->endian ){ \
        _PAR_BSWAP##bit( *((uint_fast##bit##_t*)_val) ); \
    } \
    *(ptr) = (void*)( _pval + PAR_TYPE##bit##_SIZE ); \
}while(0)

#define _PAR_PACK_NBIT_VAL( p, allocf, type, bit, v ) do { \
    void *_unused = NULL; \
    _PAR_PACK_NBIT_VAL_EX( p, &_unused, allocf, type, bit, v, 0 ); \
}while(0)


// MARK: packing floating-point number
static inline int par_pack_float32( par_pack_t *p, float num )
{
    _PAR_PACK_NBIT_VAL( p, _par_pack_increase, PAR_ISA_F, 32, num );
    return PARCEL_OK;
}
static inline int par_spack_float32( par_spack_t *p, float num )
{
    _PAR_PACK_NBIT_VAL( p, _par_pack_reduce, PAR_ISA_F, 32, num );
    return PARCEL_OK;
}

static inline int par_pack_float64( par_pack_t *p, double num )
{
    _PAR_PACK_NBIT_VAL( p, _par_pack_increase, PAR_ISA_F, 64, num );
    return PARCEL_OK;
}
static inline int par_spack_float64( par_spack_t *p, double num )
{
    _PAR_PACK_NBIT_VAL( p, _par_pack_reduce, PAR_ISA_F, 64, num );
    return PARCEL_OK;
}


// positive integer
#define _PAR_PACK_UINT( p, allocf, num ) do { \
    if( (num) <= PAR_INT6_MAX ){ \
        _PAR_PACK_TYPE( p, allocf, (uint_fast8_t)num ); \
    } \
    else if( (num) <= UINT8_MAX ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_U, 8, num ); \
    } \
    else if( (num) <= UINT16_MAX ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_U, 16, num ); \
    } \
    else if( (num) <= UINT32_MAX ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_U, 32, num ); \
    } \
    else { \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_U, 64, num ); \
    } \
}while(0)


static inline int par_pack_uint( par_pack_t *p, uint_fast64_t num )
{
    _PAR_PACK_UINT( p, _par_pack_increase, num );
    return PARCEL_OK;
}

static inline int par_spack_uint( par_spack_t *p, uint_fast64_t num )
{
    _PAR_PACK_UINT( p, _par_pack_reduce, num );
    return PARCEL_OK;
}


// idx
static inline int par_pack_idx( par_pack_t *p, uint_fast64_t idx )
{
    _PAR_PACK_TYPE( p, _par_pack_increase, PAR_ISA_IDX );
    return par_pack_uint( p, idx );
}

static inline int par_spack_idx( par_spack_t *p, uint_fast64_t idx )
{
    _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_IDX );
    return par_spack_uint( p, idx );
}


// MARK: undef _PAR_PACK_UINT
#undef _PAR_PACK_UINT


// negative integer
#define _PAR_PACK_INT( p, allocf, num ) do { \
    if( (num) > 0 ){ \
        if( (num) > INT32_MAX ){ \
            _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 64, num ); \
        } \
        else if( (num) > INT16_MAX ){ \
            _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 32, num ); \
        } \
        else if( (num) > INT8_MAX ){ \
            _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 16, num ); \
        } \
        else if( (num) > PAR_INT6_MAX ){ \
            _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 8, num ); \
        } \
        else { \
            _PAR_PACK_TYPE( p, allocf, (uint_fast8_t)num ); \
        } \
    } \
    else if( (num) < INT32_MIN ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 64, num ); \
    } \
    else if( (num) < INT16_MIN ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 32, num ); \
    } \
    else if( (num) < INT8_MIN ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 16, num ); \
    } \
    else if( (num) < PAR_INT6_MIN ){ \
        _PAR_PACK_NBIT_VAL( p, allocf, PAR_ISA_S, 8, num ); \
    } \
    else { \
        _PAR_PACK_TYPE( p, allocf, (uint_fast8_t)(-num|PAR_ISA_S6N) ); \
    } \
}while(0)


static inline int par_pack_int( par_pack_t *p, int_fast64_t num )
{
    _PAR_PACK_INT( p, _par_pack_increase, num );
    return PARCEL_OK;
}

static inline int par_spack_int( par_spack_t *p, int_fast64_t num )
{
    _PAR_PACK_INT( p, _par_pack_reduce, num );
    return PARCEL_OK;
}


// MARK: undef _PAR_PACK_INT
#undef _PAR_PACK_INT
// MARK: undef _PAR_PACK_NBIT_VAL
#undef _PAR_PACK_NBIT_VAL



// MARK: packing type with length value

#define _PAR_PACK_TYPE_WITH_LEN_EX( p, ptr, allocf, type, len, ex ) do { \
    /* 64bit */ \
    if( len & 0xFFFFFFFF00000000 ){ \
        _PAR_PACK_NBIT_VAL_EX( p, ptr, allocf, type, 64, len, ex ); \
    } \
    /* 32bit */ \
    else if( len & 0xFFFF0000 ){ \
        _PAR_PACK_NBIT_VAL_EX( p, ptr, allocf, type, 32, len, ex ); \
    } \
    /* 16bit */ \
    else if( len & 0xFF00 ){ \
        _PAR_PACK_NBIT_VAL_EX( p, ptr, allocf, type, 16, len, ex ); \
    } \
    /* 8bit */ \
    else { \
        _PAR_PACK_NBIT_VAL_EX( p, ptr, allocf, type, 8, len, ex ); \
    } \
}while(0)


#define _PAR_PACK_TYPE_WITH_LEN( p, allocf, type, len ) do { \
    void *_unused = NULL; \
    _PAR_PACK_TYPE_WITH_LEN_EX( p, &_unused, allocf, type, len, 0 ); \
}while(0)


// MARK: packing array
#define _PAR_PACK_ARRAY( p, allocf, type, len ) do { \
    if( len <= 0xF ){ \
        _PAR_PACK_TYPE( p, allocf, type##4|(uint_fast8_t)len ); \
    } \
    else { \
        _PAR_PACK_TYPE_WITH_LEN( p, allocf, type, len ); \
    } \
}while(0)


static inline int par_pack_array( par_pack_t *p, size_t len )
{
    _PAR_PACK_ARRAY( p, _par_pack_increase, PAR_ISA_ARR, len );
    return PARCEL_OK;
}

static inline int par_spack_array( par_spack_t *p, size_t len )
{
    _PAR_PACK_ARRAY( p, _par_pack_reduce, PAR_ISA_ARR, len );
    return PARCEL_OK;
}


// MARK: packing map
#define _PAR_PACK_MAP _PAR_PACK_ARRAY

static inline int par_pack_map( par_pack_t *p, size_t len )
{
    _PAR_PACK_MAP( p, _par_pack_increase, PAR_ISA_MAP, len );
    return PARCEL_OK;
}

static inline int par_spack_map( par_spack_t *p, size_t len )
{
    _PAR_PACK_MAP( p, _par_pack_reduce, PAR_ISA_MAP, len );
    return PARCEL_OK;
}

#undef _PAR_PACK_MAP
#undef _PAR_PACK_ARRAY



// MARK: packing ref

static inline int par_pack_ref( par_pack_t *p, size_t idx )
{
    if( idx < p->cur )
    {
        par_type_t *pval = (par_type_t*)( p->mem + idx );
        
        // verify type
        switch( pval->isa ){
            case PAR_ISA_REF8 ... PAR_ISA_MAP64:
            case PAR_ISA_SARR:
            case PAR_ISA_SMAP:
            case PAR_ISA_ARR4 ... PAR_ISA_MAP4_TAIL:
                _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_increase, PAR_ISA_REF, idx );
                return PARCEL_OK;
        }
    }
    
    // illegal byte sequence
    errno = PARCEL_EILSEQ;
    
    return -1;
}

static inline int par_spack_ref( par_spack_t *p, size_t idx )
{
    // illegal sequence: invalid position
    if( idx > p->cur ){
        errno = PARCEL_EILSEQ;
        return -1;
    }

    _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_reduce, PAR_ISA_REF, idx );
    return PARCEL_OK;
}


// MARK: packing raw/string
#define _PAR_PACK_BYTEA( p, allocf, type, val, len ) do { \
    void *_dest = NULL; \
    /* allocate extra bytes space */ \
    _PAR_PACK_TYPE_WITH_LEN_EX( p, &_dest, allocf, type, len, len ); \
    /* copy val */ \
    memcpy( _dest, val, len ); \
}while(0)


static inline int par_pack_raw( par_pack_t *p, void *val, size_t len )
{
    _PAR_PACK_BYTEA( p, _par_pack_increase, PAR_ISA_RAW, val, len );
    return PARCEL_OK;
}


static inline int par_pack_str( par_pack_t *p, void *val, size_t len )
{
    //*/ 5 bit length string
    if( len <= 0x1F )
    {
        void *ptr = NULL;
        // append type byte and allocate extra bytes space
        _PAR_PACK_TYPE_EX( p, _par_pack_increase, PAR_ISA_STR5|(uint8_t)len, 
                           len, &ptr );
        // copy val
        memcpy( ptr, val, len );
    }
    else {
        _PAR_PACK_BYTEA( p, _par_pack_increase, PAR_ISA_STR, val, len );
    }
    //*/
    //_PAR_PACK_BYTEA( p, _par_pack_increase, PAR_ISA_STR, val, len );
    
    return PARCEL_OK;
}


// MARK: undef _PAR_PACK_BYTEA
#undef _PAR_PACK_BYTEA


#define _PAR_SPACK_BYTEA( p, val, len ) do { \
    size_t _remain = (p)->blksize - (p)->cur; \
    /* copy to memory block if have space */ \
    if( _remain >= len ){ \
COPY2BLOCK: \
        memcpy( (p)->mem + (p)->cur, val, len ); \
        (p)->cur += len; \
    } \
    else \
    { \
        /* copy remaining bytes */ \
        if( _remain ){ \
            memcpy( (p)->mem + (p)->cur, val, _remain ); \
            (p)->cur += _remain; \
            val += _remain; \
        } \
        /* reduce memory */ \
        if( (p)->reducer( (p)->mem, (p)->cur, (p)->udata ) == 0 ) \
        { \
            /* rewind cursor */ \
            (p)->cur = 0; \
            len -= _remain; \
            /* copy to memory block if have space */ \
            if( len < (p)->blksize ){ \
                goto COPY2BLOCK; \
            } \
            /* reduce all */ \
            else if( (p)->reducer( val, len, (p)->udata ) != 0 ){ \
                return -1; \
            } \
        } \
    } \
}while(0)


static inline int par_spack_raw( par_spack_t *p, void *val, size_t len )
{
    _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_reduce, PAR_ISA_RAW, len );
    _PAR_SPACK_BYTEA( p, val, len );
    return PARCEL_OK;
}

static inline int par_spack_str( par_spack_t *p, void *val, size_t len )
{
    // 5 bit length string
    if( len <= 0x1F ){
        _PAR_PACK_TYPE( p, _par_pack_reduce, PAR_ISA_STR5|(uint8_t)len );
    }
    else {
        _PAR_PACK_TYPE_WITH_LEN( p, _par_pack_reduce, PAR_ISA_STR, len );
    }
    _PAR_SPACK_BYTEA( p, val, len );
    return PARCEL_OK;
}

// MARK: undef _PAR_SPACK_BYTEA
#undef _PAR_SPACK_BYTEA
// MARK: undef _PAR_PACK_TYPE_WITH_LEN
#undef _PAR_PACK_TYPE_WITH_LEN
// MARK: undef _PAR_PACK_TYPE_WITH_LEN_EX
#undef _PAR_PACK_TYPE_WITH_LEN_EX
// MARK: undef _PAR_PACK_SLICE
#undef _PAR_PACK_SLICE
// MARK: undef _PAR_PACK_TYPE
#undef _PAR_PACK_TYPE
// MARK: undef _PAR_PACK_TYPE_EX
#undef _PAR_PACK_TYPE_EX



// MARK: unpacking
// bin data
typedef struct {
    uint_fast8_t endian;
    size_t cur;
    size_t blksize;
    void *mem;
} par_unpack_t;


// check available block space
#define _PAR_CHECK_BLKSPC( blksize, cur, req ) do { \
    if( (cur) >= (blksize) || ( (blksize) - (cur) ) < (req) ){ \
        errno = PARCEL_ENOBLKS; \
        return -1; \
    } \
}while(0)


typedef struct {
    uint_fast8_t isa;
    union {
        uint_fast64_t len;
        uint_fast64_t idx;
    } size;
    union {
        void *bytea;
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
    p->endian = par_get_endian();
    p->cur = 0;
    p->mem = mem;
    p->blksize = blksize;
}


#define _PAR_UNPACK_NBIT_FLT( p, type, ext, bit ) do { \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, PAR_TYPE##bit##_SIZE ); \
    (ext)->val.f##bit = *(par_float##bit##_t*)( (type) + PAR_TYPE_SIZE ); \
    (p)->cur += PAR_TYPE##bit##_SIZE; \
    if( (p)->endian ){ \
        _PAR_BSWAP##bit( (ext)->val.u##bit ); \
    } \
}while(0)


#define _PAR_UNPACK_NBIT_INT( p, type, ext, bit ) do { \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, PAR_TYPE##bit##_SIZE ); \
    (ext)->val.u##bit = *(uint_fast##bit##_t*)( (type) + PAR_TYPE_SIZE ); \
    (p)->cur += PAR_TYPE##bit##_SIZE; \
    if( (bit) > 8 && (p)->endian ){ \
        _PAR_BSWAP##bit( (ext)->val.u##bit ); \
    } \
    (ext)->size.len = (ext)->val.u##bit; \
}while(0)


#define _PAR_UNPACK_NBIT_LEN( p, type, ext, bit ) \
    _PAR_UNPACK_NBIT_INT( p, type, ext, bit )


// type: PAR_ISA_RAW, PAR_ISA_STR
// len: *(uint_fast[8-64]_t*)(mem + cur + PAR_TYPE_SIZE)
// val: mem + cur + PAR_TYPE[8-64]_SIZE
#define _PAR_UNPACK_NBIT_BYTEA( p, type, ext, bit ) do { \
    _PAR_UNPACK_NBIT_LEN( p, type, ext, bit ); \
    _PAR_CHECK_BLKSPC( (p)->blksize, (p)->cur, (ext)->size.len ); \
    (ext)->val.bytea = (p)->mem + (p)->cur; \
    (p)->cur += (ext)->size.len; \
}while(0)


static inline int par_unpack( par_unpack_t *p, par_extract_t *ext )
{
    if( p->cur < p->blksize )
    {
        par_type_t *type = (par_type_t*)p->mem + p->cur;
        
        // init
        ext->isa = type->isa;
        ext->size.len = 0;
        if( ext->isa <= PAR_ISA_S6N_TAIL )
        {
            // small negative integer
            if( ext->isa & PAR_ISA_S6N ){
                ext->val.i8 = -((int_fast8_t)(ext->isa & ~PAR_ISA_S6N));
            }
            // small positive integer
            else {
                ext->val.i8 = (int_fast8_t)type->isa;
            }
            ext->isa = PAR_ISA_S6;
            p->cur += PAR_TYPE_SIZE;
            return PARCEL_OK;
        }
        // 7 bit integer
        switch( ext->isa )
        {
            // 1 byte types
            case PAR_ISA_NIL:
            case PAR_ISA_TRUE:
            case PAR_ISA_FALSE:
            case PAR_ISA_PINF:
            case PAR_ISA_NINF:
            case PAR_ISA_NAN:
            case PAR_ISA_SARR:
            case PAR_ISA_SMAP:
            case PAR_ISA_IDX:
            case PAR_ISA_EOS:
                p->cur += PAR_TYPE_SIZE;
            break;

            // 8 bit
            case PAR_ISA_U8:
            case PAR_ISA_S8:
                _PAR_UNPACK_NBIT_INT( p, type, ext, 8 );
            break;
            
            case PAR_ISA_REF8:
            case PAR_ISA_ARR8:
            case PAR_ISA_MAP8:
                _PAR_UNPACK_NBIT_LEN( p, type, ext, 8 );
            break;
            
            case PAR_ISA_RAW8:
            case PAR_ISA_STR8:
                _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 8 );
            break;
            
            // 16 bit
            case PAR_ISA_U16:
            case PAR_ISA_S16:
                _PAR_UNPACK_NBIT_INT( p, type, ext, 16 );
            break;
            
            case PAR_ISA_REF16:
            case PAR_ISA_ARR16:
            case PAR_ISA_MAP16:
                _PAR_UNPACK_NBIT_LEN( p, type, ext, 16 );
            break;
            
            case PAR_ISA_RAW16:
            case PAR_ISA_STR16:
                _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 16 );
            break;
            
            // 32 bit
            case PAR_ISA_U32:
            case PAR_ISA_S32:
                _PAR_UNPACK_NBIT_INT( p, type, ext, 32 );
            break;
            
            case PAR_ISA_F32:
                _PAR_UNPACK_NBIT_FLT( p, type, ext, 32 );
            break;
            
            case PAR_ISA_REF32:
            case PAR_ISA_ARR32:
            case PAR_ISA_MAP32:
                _PAR_UNPACK_NBIT_LEN( p, type, ext, 32 );
            break;

            case PAR_ISA_RAW32:
            case PAR_ISA_STR32:
                _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 32 );
            break;
            
            // 64 bit
            case PAR_ISA_U64:
            case PAR_ISA_S64:
                _PAR_UNPACK_NBIT_INT( p, type, ext, 64 );
            break;
            
            case PAR_ISA_F64:
                _PAR_UNPACK_NBIT_FLT( p, type, ext, 64 );
            break;
            
            case PAR_ISA_REF64:
            case PAR_ISA_ARR64:
            case PAR_ISA_MAP64:
                _PAR_UNPACK_NBIT_LEN( p, type, ext, 64 );
            break;
            
            case PAR_ISA_RAW64:
            case PAR_ISA_STR64:
                _PAR_UNPACK_NBIT_BYTEA( p, type, ext, 64 );
            break;
            
            // 5 bit length string
            case PAR_ISA_STR5 ... PAR_ISA_STR5_TAIL:
                ext->isa &= ~(0x1F);
                ext->size.len = type->isa & 0x1F;
                ext->val.bytea = p->mem + p->cur + PAR_TYPE_SIZE;
                p->cur += PAR_TYPE_SIZE + ext->size.len;
            break;
            
            // 4 bit length array/map
            case PAR_ISA_ARR4 ... PAR_ISA_MAP4_TAIL:
                ext->isa &= ~(0xF);
                ext->size.len = type->isa & 0xF;
                p->cur += PAR_TYPE_SIZE;
            break;

            // illegal byte sequence
            default:
                errno = PARCEL_EILSEQ;
                return -1;
        }
        
        return PARCEL_OK;
    }
    
    // end-of-data
    // no message available on memory block
    errno = PARCEL_ENODATA;
    return -2;
}


// unpack an index value of non-consecutive array
static inline int par_unpack_idx( par_unpack_t *p, par_extract_t *ext )
{
    int rc = par_unpack( p, ext );
    
    if( rc == 0 )
    {
        // check value type
        switch( ext->isa ){
            case PAR_ISA_S6:
            case PAR_ISA_U8 ... PAR_ISA_U64:
                return rc;
        }
        // illegal byte sequence
        errno = PARCEL_EILSEQ;
        return -1;
    }
    
    return rc;
}


// unpack a key value of map
static inline int par_unpack_key( par_unpack_t *p, par_extract_t *ext, 
                                  int allow_eos )
{
    int rc = par_unpack( p, ext );
    
    if( rc == 0 )
    {
        // check value type
        switch( ext->isa ){
            case PAR_ISA_S6:
            case PAR_ISA_U8 ... PAR_ISA_S64:
            case PAR_ISA_STR5:
            case PAR_ISA_STR8 ... PAR_ISA_STR64:
                return PARCEL_OK;
            
            case PAR_ISA_EOS:
                if( allow_eos ){
                    return PAR_ISA_EOS;
                }
        }
        // illegal byte sequence
        errno = PARCEL_EILSEQ;
    }
    
    return rc;
}


// MARK: undef _PAR_CHECK_BLKSPC
#undef _PAR_CHECK_BLKSPC
// MARK: undef _PAR_UNPACK_*
#undef _PAR_UNPACK_NBIT_FLT
#undef _PAR_UNPACK_NBIT_INT
#undef _PAR_UNPACK_NBIT_LEN
#undef _PAR_UNPACK_NBIT_BYTEA

// MARK: undef PAR_TYPE*_SIZE
#undef PAR_TYPE_SIZE
#undef PAR_TYPE8_SIZE
#undef PAR_TYPE16_SIZE
#undef PAR_TYPE32_SIZE
#undef PAR_TYPE64_SIZE

// MARK: undef _PAR_BSWAP*
#undef _PAR_BSWAP8
#undef _PAR_BSWAP64
#undef _PAR_BSWAP32
#undef _PAR_BSWAP16


#endif
