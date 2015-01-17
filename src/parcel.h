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

#ifndef ___LUA_PARCEL___
#define ___LUA_PARCEL___


#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>


// MARK: endian
static inline uint_fast8_t par_swap8( uint_fast8_t val )
{
    return val;
}

static inline uint_fast16_t par_swap16( uint_fast16_t val )
{
    return (uint_fast16_t)(val>>8) | (uint_fast16_t)(val<<8);
}

static inline uint_fast32_t par_swap32( uint_fast32_t val )
{
    return val;
}


// MARK: parcel format

/*
    TYPE
    ----------------
    0
    0 1 2 3 4 5 6 7
    +--------------+
    kind       |info
    +--------------+
*/


#define PAR_INFO_FIELDS \
    uint_fast8_t endian:1; \
    uint_fast8_t flag:1; \
    uint_fast8_t kind:6

// endianness
#define PAR_E_LIT    0
#define PAR_E_BIG    0x1

// flag
// no flag
#define PAR_F_NONE      0x0

// boolean
#define PAR_F_FALSE     PAR_F_NONE
#define PAR_F_TRUE      0x1

// signedness
#define PAR_F_UNSIGN    PAR_F_NONE
#define PAR_F_SIGNED    0x1

// lengthiness
#define PAR_F_FIXLEN    PAR_F_NONE
#define PAR_F_VARLEN    0x1


// kind(6): 0-63
enum {
    // 1 byte types
    PAR_K_NIL = 0,
    // flag: boolean
    PAR_K_BOL,
    PAR_K_TBL,
    PAR_K_NAN,
    // flag: signedness
    PAR_K_INF,
    PAR_K_I0,
    
    // 8 byte types
    // flag: lengthiness
    PAR_K_STR,
    PAR_K_ARR,
    PAR_K_MAP,
    
    // 2-9 byte
    // endian: endianness
    // flag: signedness
    PAR_K_I8,
    PAR_K_I16,
    PAR_K_I32,
    PAR_K_I64,
    PAR_K_F64
};


typedef union {
    uint_fast8_t size[1];
    struct {
        PAR_INFO_FIELDS;
    } data;
} par_type_t;


// NIL
typedef par_type_t par_nil_t;

// BOOLEAN
typedef par_type_t par_bol_t;

// EMPTY TABLE
typedef par_type_t par_tbl_t;

// NAN
typedef par_type_t par_nan_t;

// INF
typedef par_type_t par_inf_t;

// ZERO NUMBER
typedef par_type_t par_type0_t;


#define PAR_TYPEX_MAXLEN    0xFFFFFFFFFFFFFFULL

typedef union {
    uint_fast8_t size[8];
    struct {
        PAR_INFO_FIELDS;
        uint_fast64_t len:56;
    } data;
} par_typex_t;

// STRING
typedef par_typex_t par_str_t;

// ARRAY
typedef par_typex_t par_arr_t;

// MAP
typedef par_typex_t par_map_t;


typedef union {
    uint_fast8_t size[2];
    struct {
        PAR_INFO_FIELDS;
        uint_fast8_t val[1];
    } data;
} par_type8_t;


typedef union {
    uint_fast8_t size[3];
    struct {
        PAR_INFO_FIELDS;
        uint_fast8_t val[2];
    } data;
} par_type16_t;


typedef union {
    uint_fast8_t size[5];
    struct {
        PAR_INFO_FIELDS;
        uint_fast8_t val[4];
    } data;
} par_type32_t;


typedef union {
    uint_fast8_t size[9];
    struct {
        PAR_INFO_FIELDS;
        uint_fast8_t val[8];
    } data;
} par_type64_t;


// to use unpack
typedef double par_float64_t;

typedef union {
    uint_fast8_t size[16];
    struct {
        PAR_INFO_FIELDS;
        uint_fast64_t len:56;
        union  {
            char *str;
            uint_fast8_t u8;
            uint_fast16_t u16;
            uint_fast32_t u32;
            uint_fast64_t u64;
            int_fast8_t i8;
            int_fast16_t i16;
            int_fast32_t i32;
            int_fast64_t i64;
            par_float64_t f64;
        } val;
    } data;
} par_extract_t;

#undef PAR_INFO_FIELDS


// MARK: parcel memory
#define PAR_DEFAULT_BLK_SIZE    1024


// MARK: packing

// bin data
typedef struct {
    size_t cur;
    size_t blksize;
    size_t nblkmax;
    size_t nblk;
    size_t bytes;
    void *mem;
} parcel_pack_t;


static inline int par_pack_init( parcel_pack_t *p, size_t blksize )
{
    if( !blksize ){
        blksize = PAR_DEFAULT_BLK_SIZE;
    }
    else
    {
        if( blksize < 16 ){
            blksize = 16;
        }
        blksize = blksize / 16 * 16;
    }
    
    if( ( p->mem = malloc( blksize ) ) )
    {
        p->cur = 0;
        p->blksize = blksize;
        p->nblkmax = SIZE_MAX / blksize;
        p->nblk = 1;
        p->bytes = blksize;
        return 0;
    }
    
    return -1;
}


static inline void par_pack_dispose( parcel_pack_t *p )
{
    if( p->mem ){
        free( p->mem );
        p->mem = NULL;
    }
}


static inline int _par_pack_increase( parcel_pack_t *p, size_t bytes )
{
    size_t remain = p->bytes - p->cur;

    // remain < bytes
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
            if( ( mem = realloc( p->mem, bytes ) ) ){
                p->mem = mem;
                p->nblk += nblk;
                p->bytes = bytes;
                return 0;
            }
        }
        else {
            errno = ENOMEM;
        }
        
        return -1;
    }
    
    return 0;
}


static inline void *_par_pack_update_cur( parcel_pack_t *p, size_t bytes )
{
    void *mem = p->mem + p->cur;
    
    p->cur += bytes;
    
    return mem;
}


// allocate sizeof(t) and extra bytes
#define _par_pack_slice_ex(p,l) ({ \
    if( _par_pack_increase( p, l ) == -1 ){ \
        return -1; \
    } \
    _par_pack_update_cur( p, l ); \
})


// allocate sizeof(t) bytes
#define _par_pack_slice(p,t)  (t*)_par_pack_slice_ex(p,sizeof(t))


static inline int _par_pack_type( parcel_pack_t *p, uint8_t kind, uint8_t flag )
{
    par_type_t *pval = _par_pack_slice( p, par_type_t );
    
    pval->data.kind = kind;
    pval->data.flag = flag;
    
    return 0;
}

#define par_pack_bool(p,val)    _par_pack_type(p,PAR_K_BOL,!!val)
#define par_pack_nil(p)         _par_pack_type(p,PAR_K_NIL,0)
#define par_pack_tbl(p)         _par_pack_type(p,PAR_K_TBL,0)
#define par_pack_nan(p)         _par_pack_type(p,PAR_K_NAN,0)
#define par_pack_inf(p,sign)    _par_pack_type(p,PAR_K_INF, sign )
#define par_pack_zero(p)        _par_pack_type(p,PAR_K_I0,0)


static inline int par_pack_str( parcel_pack_t *p, void *val, size_t len )
{
    par_str_t *pval = _par_pack_slice( p, par_str_t );
    void *dest = NULL;
    
    pval->data.kind = PAR_K_STR;
    pval->data.flag = PAR_F_FIXLEN;
    pval->data.len = len;
    
    // allocate extra bytes space
    dest = _par_pack_slice_ex( p, len );
    memcpy( dest, val, len );
    
    return 0;
}


static inline int _par_pack_typex( parcel_pack_t *p, uint8_t kind, size_t *idx )
{
    par_typex_t *pval = NULL;
    
    *idx = p->cur;
    pval = _par_pack_slice( p, par_typex_t );
    pval->data.kind = kind;
    pval->data.flag = PAR_F_FIXLEN;
    
    return 0;
}

#define par_pack_arr(p,idx)     _par_pack_typex(p,PAR_K_ARR,idx)
#define par_pack_map(p,idx)     _par_pack_typex(p,PAR_K_MAP,idx)


static inline int par_pack_tbllen( parcel_pack_t *p, size_t idx, size_t len )
{
    par_typex_t *pval = NULL;
    
    if( ( SIZE_MAX - idx ) < sizeof( par_typex_t ) ||
        ( idx + sizeof( par_typex_t ) ) > p->cur ){
        errno = EDOM;
        return -1;
    }
    else if( len > PAR_TYPEX_MAXLEN ){
        errno = EINVAL;
        return -1;
    }
    
    pval = (par_typex_t*)(p->mem + idx);
    switch( pval->data.kind ){
        case PAR_K_ARR:
        case PAR_K_MAP:
            pval->data.len = len;
            return 0;
    }
    
    // invalid type
    errno = EINVAL;
    
    return -1;
}


#define _par_pack_bitint(p,bit,v,f) do { \
    par_type ## bit ## _t *pval = _par_pack_slice( p, par_type ## bit ## _t ); \
    pval->data.kind = PAR_K_I##bit; \
    pval->data.flag = (uint_fast8_t)f; \
    *((uint_fast ## bit ## _t*)pval->data.val) = (uint_fast ## bit ## _t)v; \
}while(0)


// positive integer
static inline int par_pack_uint( parcel_pack_t *p, uint_fast64_t num )
{
    if( num == 0 ){
        return par_pack_zero( p );
    }
    else if( num <= UINT8_MAX ){
        _par_pack_bitint( p, 8, num, PAR_F_UNSIGN );
    }
    else if( num <= UINT16_MAX ){
        _par_pack_bitint( p, 16, num, PAR_F_UNSIGN );
    }
    else if( num <= UINT32_MAX ){
        _par_pack_bitint( p, 32, num, PAR_F_UNSIGN );
    }
    else {
        _par_pack_bitint( p, 64, num, PAR_F_UNSIGN );
    }
    
    return 0;
}


// negative integer
static inline int par_pack_int( parcel_pack_t *p, int_fast64_t num )
{
    if( num == 0 ){
        return par_pack_zero( p );
    }
    else if( num >= INT8_MIN ){
        _par_pack_bitint( p, 8, num, PAR_F_SIGNED );
    }
    else if( num >= INT16_MIN ){
        _par_pack_bitint( p, 16, num, PAR_F_SIGNED );
    }
    else if( num >= INT32_MIN ){
        _par_pack_bitint( p, 32, num, PAR_F_SIGNED );
    }
    else {
        _par_pack_bitint( p, 64, num, PAR_F_SIGNED );
    }
    
    return 0;
}


#define _par_pack_bitfloat(p,bit,v,f) do { \
    par_type ## bit ## _t *pval = _par_pack_slice( p, par_type ## bit ## _t ); \
    pval->data.kind = PAR_K_F##bit; \
    pval->data.flag = (uint_fast8_t)f; \
    *((par_float ## bit ## _t*)pval->data.val) = (par_float ## bit ## _t)v; \
}while(0)


// float
static inline int par_pack_float( parcel_pack_t *p, double num )
{
    if( num == 0.0 ){
        return par_pack_zero( p );
    }
    else if( num > 0.0 ){
        _par_pack_bitfloat( p, 64, num, PAR_F_UNSIGN );
    }
    else {
        _par_pack_bitfloat( p, 64, num, PAR_F_SIGNED );
    }
    
    return 0;
}


// MARK: unpacking
// bin data
typedef struct {
    size_t cur;
    size_t blksize;
    void *mem;
} parcel_unpack_t;


#define _par_unpack_bitint(cur,t,bit,ext) do { \
    par_type ## bit ## _t *pval = (par_type ## bit ## _t*)t; \
    ext->data.endian = pval->data.endian; \
    ext->data.flag = pval->data.flag; \
    if( ext->data.flag ){ \
        ext->data.val.i##bit = *(int_fast ## bit ## _t*)pval->data.val; \
    } \
    else { \
        ext->data.val.u##bit = *(uint_fast ## bit ## _t*)pval->data.val; \
    } \
    *(cur) += sizeof( par_type ## bit ## _t ); \
}while(0)


#define _par_unpack_bitfloat(cur,t,bit,ext) do { \
    par_type ## bit ## _t *pval = (par_type ## bit ## _t*)t; \
    ext->data.endian = pval->data.endian; \
    ext->data.flag = pval->data.flag; \
    ext->data.val.f##bit = *(par_float ## bit ## _t*)pval->data.val; \
    *(cur) += sizeof( par_type ## bit ## _t ); \
}while(0)


static inline int par_unpack( parcel_unpack_t *p, par_extract_t *ext )
{
    par_type_t *type = NULL;

    if( p->cur < p->blksize )
    {
        type = (par_type_t*)p->mem + p->cur;
        
        ext->data.kind = type->data.kind;
        // 1 byte types
        if( type->data.kind <= PAR_K_I0 )
        {
            // set flag value
            if( type->data.kind == PAR_K_BOL || type->data.kind == PAR_K_INF ){
                ext->data.flag = type->data.flag;
            }
            p->cur += sizeof( par_type_t );
        }
        // 8 byte types
        else if( type->data.kind <= PAR_K_MAP )
        {
            par_typex_t *typex = (par_typex_t*)type;
            
            ext->data.len = typex->data.len;
            // set string value
            // str = head: mem + par_typex_t, tail: head + len
            if( type->data.kind == PAR_K_STR ){
                ext->data.val.str = (char*)p->mem + p->cur + sizeof( par_typex_t );
                // move cursor
                p->cur += typex->data.len;
            }
            p->cur += sizeof( par_typex_t );
        }
        // number
        // endian = 1:big-endian, 0:littel-endian
        // flag = 1:sign, 0:unsign
        else if( type->data.kind == PAR_K_I8 ){
            _par_unpack_bitint( &p->cur, type, 8, ext );
        }
        else if( type->data.kind == PAR_K_I16 ){
            _par_unpack_bitint( &p->cur, type, 16, ext );
        }
        else if( type->data.kind == PAR_K_I32 ){
            _par_unpack_bitint( &p->cur, type, 32, ext );
        }
        else if( type->data.kind == PAR_K_I64 ){
            _par_unpack_bitint( &p->cur, type, 64, ext );
        }
        else if( type->data.kind == PAR_K_F64 ){
            _par_unpack_bitfloat( &p->cur, type, 64, ext );
        }
        // unknown data type
        else {
            errno = EINVAL;
            return -1;
        }
        
        return 1;
    }
    
    // end-of-data
    
    return 0;
}


#endif
