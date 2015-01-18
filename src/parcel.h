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


#define PAR_INFO_FIELDS \
    uint_fast8_t endian:1; \
    uint_fast8_t flag:2; \
    uint_fast8_t kind:5

// endianness
#define PAR_E_LIT    0x0
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

// byte array size
#define PAR_F_SIZE8     PAR_F_NONE
#define PAR_F_SIZE16    0x1
#define PAR_F_SIZE32    0x2
#define PAR_F_SIZE64    0x3

// array/map length
#define PAR_F_FIXLEN    PAR_F_NONE
#define PAR_F_VARLEN    0x1


// kind(6): 0-63
enum {
    // 1 byte types
    PAR_K_NIL = 0,
    // flag: boolean
    PAR_K_BOL,
    PAR_K_TBL,
    PAR_K_EOS,  // end-of-stream
    PAR_K_NAN,
    // flag: signedness
    PAR_K_INF,
    PAR_K_I0,
    
    // 1+[0-8] byte types
    // flag: byte array size
    PAR_K_STR,
    
    // 1+8 byte types
    // flag: array/map length
    // NOTE: should append PAR_K_EOS packet if flag is PAR_F_VARLEN
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


/*
    TYPE
    1 -+------------------
      7| endian
      6| flag
      5|
      4| kind
      3|
      2|
      1|
      0|
    0 -+------------------
    
    kind        flag
    ----------+------------------------------
    PAR_K_NIL |
    PAR_K_BOL |
    PAR_K_TBL |
    PAR_K_EOS |
    PAR_K_NAN |
    ----------+------------------------------
    PAR_K_INF | PAR_F_UNSIGN | PAR_F_SIGNED
    ----------+------------------------------
    PAR_K_I0  |
    ----------+------------------------------
    PAR_K_STR | PAR_F_SIZE[8-64]
    ----------+------------------------------
    PAR_K_ARR | PAR_F_FIXLEN | PAR_F_VARLEN
    PAR_K_MAP |
    ----------+------------------------------
    PAR_K_I8  | PAR_F_UNSIGN | PAR_F_SIGNED
    PAR_K_I16 |
    PAR_K_I32 |
    PAR_K_I64 |
    PAR_K_F64 |
    ----------+------------------------------
*/
typedef union {
    uint_fast8_t size[1];
    struct {
        PAR_INFO_FIELDS;
    } data;
} par_type_t;


#define PAR_TYPEX_MAXLEN    0xFFFFFFFFFFFFFFFULL

typedef union {
    uint_fast8_t size[9];
    struct {
        PAR_INFO_FIELDS;
        uint_fast8_t len[8];
    } data;
} par_typex_t;

typedef uint_fast64_t par_typelen_t;


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


// par_type[8-64]_t size
#define PAR_TYPE_SIZE       sizeof(par_type_t)
#define PAR_TYPEX_SIZE      sizeof(par_typex_t)
#define PAR_TYPE8_SIZE      sizeof(par_type8_t)
#define PAR_TYPE16_SIZE     sizeof(par_type16_t)
#define PAR_TYPE32_SIZE     sizeof(par_type32_t)
#define PAR_TYPE64_SIZE     sizeof(par_type64_t)


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


// MARK: default memory block size
#define PAR_DEFAULT_BLK_SIZE    1024


// MARK: packing

// bin data
typedef int (*par_packreduce_t)( void *mem, size_t bytes, void *udata );

typedef struct {
    uint8_t endian;
    size_t cur;
    size_t blksize;
    size_t nblkmax;
    size_t nblk;
    size_t bytes;
    void *mem;
    // stream
    par_packreduce_t reducer;
    void *udata;
} parcel_pack_t;


static inline int par_pack_init( parcel_pack_t *p, size_t blksize, 
                                 par_packreduce_t reducer, void *udata )
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
        static const int endian = 1;
        
        // 0: little-endian, 1: big-endian
        p->endian = !(*(char*)&endian);
        p->cur = 0;
        p->blksize = blksize;
        p->nblkmax = SIZE_MAX / blksize;
        p->nblk = 1;
        p->bytes = blksize;
        p->reducer = reducer;
        p->udata = udata;
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
    if( p->reducer )
    {
        size_t remain = p->blksize - p->cur;
        
        // remain >= bytes
        if( remain >= bytes ){
            return 0;
        }
        // reduce memory
        else if( p->reducer( p->mem, p->cur, p->udata ) == 0 ){
            // rewind cursor
            p->cur = 0;
            return 0;
        }
    }
    else
    {
        size_t remain = p->bytes - p->cur;
        
        // remain >= bytes
        if( remain >= bytes ){
            return 0;
        }
        else
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
        }
    }
    
    return -1;
}


static inline void *_par_pack_update_cur( parcel_pack_t *p, size_t bytes )
{
    void *mem = p->mem + p->cur;
    
    p->cur += bytes;
    
    return mem;
}


// allocate sizeof(t) and extra bytes
#define _par_pack_slice(p,l) ({ \
    if( _par_pack_increase( p, l ) == -1 ){ \
        return -1; \
    } \
    _par_pack_update_cur( p, l ); \
})


static inline int _par_pack_type( parcel_pack_t *p, uint8_t kind, uint8_t flag )
{
    par_type_t *pval = _par_pack_slice( p, PAR_TYPE_SIZE );
    
    pval->data.endian = p->endian;
    pval->data.kind = kind;
    pval->data.flag = flag;
    
    return 0;
}

#define par_pack_bool(p,val)    _par_pack_type(p,PAR_K_BOL,!!val)
#define par_pack_nil(p)         _par_pack_type(p,PAR_K_NIL,0)
#define par_pack_tbl(p)         _par_pack_type(p,PAR_K_TBL,0)
#define par_pack_eos(p)         _par_pack_type(p,PAR_K_EOS,0)
#define par_pack_nan(p)         _par_pack_type(p,PAR_K_NAN,0)
#define par_pack_inf(p,sign)    _par_pack_type(p,PAR_K_INF, sign )
#define par_pack_zero(p)        _par_pack_type(p,PAR_K_I0,0)


static inline int par_pack_str( parcel_pack_t *p, void *val, size_t len )
{
    par_type_t *pval = NULL;
    
    // check val size
    // 64bit
    if( len & 0xFFFFFFFF00000000 ){
        pval = _par_pack_slice( p, PAR_TYPE64_SIZE );
        *(uint_fast64_t*)(pval+1) = (uint_fast64_t)len;
        pval->data.flag = PAR_F_SIZE64;
    }
    // 32bit
    else if( len & 0xFFFF0000 ){
        pval = _par_pack_slice( p, PAR_TYPE32_SIZE );
        *(uint_fast32_t*)(pval+1) = (uint_fast32_t)len;
        pval->data.flag = PAR_F_SIZE32;
    }
    // 16bit
    else if( len & 0xFF00 ){
        pval = _par_pack_slice( p, PAR_TYPE16_SIZE );
        *(uint_fast16_t*)(pval+1) = (uint_fast16_t)len;
        pval->data.flag = PAR_F_SIZE16;
    }
    // 8bit
    else {
        pval = _par_pack_slice( p, PAR_TYPE8_SIZE );
        *(uint_fast8_t*)(pval+1) = (uint_fast8_t)len;
        pval->data.flag = PAR_F_SIZE8;
    }
    pval->data.endian = p->endian;
    pval->data.kind = PAR_K_STR;
    
    if( p->reducer )
    {
        size_t remain = p->blksize - p->cur;
        
        // copy to memory block if have space
        if( remain >= len ){
COPY2BLOCK:
            memcpy( p->mem + p->cur, val, len );
            p->cur += len;
            return 0;
        }
        
        // no-space
        memcpy( p->mem + p->cur, val, remain );
        p->cur += remain;
        val += remain;
        // reduce memory
        if( p->reducer( p->mem, p->cur, p->udata ) == 0 )
        {
            // rewind cursor
            p->cur = 0;
            len -= remain;
            // copy to memory block if have space
            if( len < p->blksize ){
                goto COPY2BLOCK;
            }
            // reduce 
            else if( p->reducer( val, len, p->udata ) == 0 ){
                return 0;
            }
        }
        
        return -1;
    }
    // allocate extra bytes space
    else {
        void *dest = _par_pack_slice( p, len );
        memcpy( dest, val, len );
    }
    
    return 0;
}


static inline int _par_pack_typex( parcel_pack_t *p, uint8_t kind, size_t *idx )
{
    if( !p->reducer )
    {
        par_type_t *pval = NULL;
        
        *idx = p->cur;
        pval = _par_pack_slice( p, PAR_TYPEX_SIZE );
        pval->data.endian = p->endian;
        pval->data.flag = PAR_F_FIXLEN;
        pval->data.kind = kind;
        
        return 0;
    }
    
    return _par_pack_type( p, kind, PAR_F_VARLEN );
}

#define par_pack_arr(p,idx)     _par_pack_typex(p,PAR_K_ARR,idx)
#define par_pack_map(p,idx)     _par_pack_typex(p,PAR_K_MAP,idx)


static inline int par_pack_tbllen( parcel_pack_t *p, size_t idx, size_t len )
{
    // append eos
    if( p->reducer ){
        return par_pack_eos( p );
    }
    else
    {
        par_type_t *pval = NULL;
        
        if( idx > p->cur || ( idx + PAR_TYPEX_SIZE ) > p->cur ){
            errno = EDOM;
            return -1;
        }
        else if( len > PAR_TYPEX_MAXLEN ){
            errno = EINVAL;
            return -1;
        }
        
        pval = (par_type_t*)(p->mem + idx);
        switch( pval->data.kind ){
            case PAR_K_ARR:
            case PAR_K_MAP:
                if( pval->data.flag == PAR_F_FIXLEN ){
                    par_typelen_t *plen = (par_typelen_t*)(pval+1);
                    *plen = len;
                    return 0;
                }
        }
        
        // invalid type
        errno = EINVAL;
    }
    
    return -1;
}


#define _par_pack_bitint(p,bit,v,f) do { \
    par_type_t *pval = _par_pack_slice( p, PAR_TYPE ## bit ## _SIZE ); \
    pval->data.endian = p->endian; \
    pval->data.flag = (uint_fast8_t)f; \
    pval->data.kind = PAR_K_I##bit; \
    *((uint_fast ## bit ## _t*)(pval+1)) = (uint_fast ## bit ## _t)v; \
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
    par_type_t *pval = _par_pack_slice( p, PAR_TYPE ## bit ## _SIZE ); \
    pval->data.endian = p->endian; \
    pval->data.flag = (uint_fast8_t)f; \
    pval->data.kind = PAR_K_F##bit; \
    *((par_float ## bit ## _t*)(pval+1)) = (par_float ## bit ## _t)v; \
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
        
        // set endian, kind and flag value
        ext->data.kind = type->data.kind;
        ext->data.flag = type->data.flag;
        ext->data.endian = type->data.endian;
        // 1 byte types
        if( type->data.kind <= PAR_K_I0 ){
            p->cur += sizeof( par_type_t );
        }
        // 8 byte types
        else if( type->data.kind == PAR_K_STR )
        {
            // len: *(uint_fast[8-64]_t*)(mem + cur + PAR_TYPE_SIZE)
            // val: mem + cur + PAR_TYPE[8-64]_SIZE
            switch( ext->data.flag ){
                case PAR_F_SIZE8:
                    ext->data.len = *(uint_fast8_t*)(type+1);
                    ext->data.val.str = (char*)(type+PAR_TYPE8_SIZE);
                    p->cur += PAR_TYPE8_SIZE + ext->data.len;
                break;
                case PAR_F_SIZE16:
                    ext->data.len = *(uint_fast16_t*)(type+1);
                    ext->data.val.str = (char*)(type+PAR_TYPE16_SIZE);
                    p->cur += PAR_TYPE16_SIZE + ext->data.len;
                break;
                case PAR_F_SIZE32:
                    ext->data.len = *(uint_fast32_t*)(type+1);
                    ext->data.val.str = (char*)(type+PAR_TYPE32_SIZE);
                    p->cur += PAR_TYPE32_SIZE + ext->data.len;
                break;
                case PAR_F_SIZE64:
                    ext->data.len = *(uint_fast64_t*)(type+1);
                    ext->data.val.str = (char*)(type+PAR_TYPE64_SIZE);
                    p->cur += PAR_TYPE64_SIZE + ext->data.len;
                break;
                default:
                    errno = EINVAL;
                    return -1;
            }
        }
        else if( type->data.kind <= PAR_K_MAP )
        {
            if( type->data.flag == PAR_F_FIXLEN ){
                ext->data.len = *((par_typelen_t*)(type+1));
                p->cur += PAR_TYPEX_SIZE;
            }
            else {
                ext->data.len = 0;
                p->cur += sizeof( par_type_t );
            }
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
