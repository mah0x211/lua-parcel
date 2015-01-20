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
    PARCEL_OK = 0,
    // ENOMEM: cannot allocate memory
    PARCEL_ENOMEM = ENOMEM,
    // ENOBUFS: no memory block space available
    PARCEL_ENOBLKS = ENOBUFS,
    // EILSEQ: illegal byte sequence
    PARCEL_EILSEQ = EILSEQ,
    
    // packing
    // EDOM: index argument out of domain
    PARCEL_EDOM = EDOM,
    
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
            return "index argument out of domain";
        
        // unpacking error
        case PARCEL_ENODATA:
            return "no data available on memory block";
        
        default:
            return strerror( errno );
    }
}


// MARK: parcel format

#define PAR_INFO_FIELDS \
    uint_fast8_t endian:1; \
    uint_fast8_t flag:2; \
    uint_fast8_t kind:5


#define PAR_INFO_BYTE_FIELDS \
    uint_fast8_t endian; \
    uint_fast8_t flag; \
    uint_fast8_t kind


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

#define _par_fsize2byte(f)  (1<<(f))

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
    
    // 1+[1-8] byte types
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


// to use unpack
typedef double par_float64_t;

typedef struct {
    PAR_INFO_BYTE_FIELDS;
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
        par_float64_t f64;
    } val;
} par_extract_t;

#undef PAR_INFO_FIELDS
#undef PAR_INFO_BYTE_FIELDS


// MARK: default memory block size
#define PAR_DEFAULT_BLK_SIZE    1024


// check available block space
#define _par_check_blkspc(blksize,cur,req) do { \
    if( (cur) >= (blksize) || ((blksize)-(cur)) < (req) ){ \
        errno = PARCEL_ENOBLKS; \
        return -1; \
    } \
}while(0)


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
} par_pack_t;


static inline int par_pack_init( par_pack_t *p, size_t blksize, 
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


static inline void par_pack_dispose( par_pack_t *p )
{
    if( p->mem ){
        free( p->mem );
        p->mem = NULL;
    }
}


static inline int _par_pack_increase( par_pack_t *p, size_t bytes )
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
                errno = PARCEL_ENOMEM;
            }
        }
    }
    
    return -1;
}


static inline void *_par_pack_update_cur( par_pack_t *p, size_t bytes )
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


static inline int _par_pack_type( par_pack_t *p, uint8_t kind, uint8_t flag )
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


static inline int par_pack_str( par_pack_t *p, void *val, size_t len )
{
    par_type_t *pval = NULL;
    
    // check val size
    // 64bit
    if( len & 0xFFFFFFFF00000000 ){
        pval = _par_pack_slice( p, PAR_TYPE64_SIZE );
        *(uint_fast64_t*)(pval+PAR_TYPE_SIZE) = (uint_fast64_t)len;
        pval->data.flag = PAR_F_SIZE64;
    }
    // 32bit
    else if( len & 0xFFFF0000 ){
        pval = _par_pack_slice( p, PAR_TYPE32_SIZE );
        *(uint_fast32_t*)(pval+PAR_TYPE_SIZE) = (uint_fast32_t)len;
        pval->data.flag = PAR_F_SIZE32;
    }
    // 16bit
    else if( len & 0xFF00 ){
        pval = _par_pack_slice( p, PAR_TYPE16_SIZE );
        *(uint_fast16_t*)(pval+PAR_TYPE_SIZE) = (uint_fast16_t)len;
        pval->data.flag = PAR_F_SIZE16;
    }
    // 8bit
    else {
        pval = _par_pack_slice( p, PAR_TYPE8_SIZE );
        *(uint_fast8_t*)(pval+PAR_TYPE_SIZE) = (uint_fast8_t)len;
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


static inline int _par_pack_typex( par_pack_t *p, uint8_t kind, size_t *idx )
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


static inline int par_pack_tbllen( par_pack_t *p, size_t idx, size_t len )
{
    // append eos
    if( p->reducer ){
        return par_pack_eos( p );
    }
    else
    {
        par_type_t *pval = NULL;
        
        if( idx > p->cur || ( idx + PAR_TYPEX_SIZE ) > p->cur ){
            errno = PARCEL_EDOM;
            return -1;
        }
        
        pval = (par_type_t*)(p->mem + idx);
        switch( pval->data.kind ){
            case PAR_K_ARR:
            case PAR_K_MAP:
                if( pval->data.flag == PAR_F_FIXLEN ){
                    par_typelen_t *plen = (par_typelen_t*)(pval+PAR_TYPE_SIZE);
                    *plen = len;
                    return 0;
                }
        }
        
        // illegal byte sequence
        errno = PARCEL_EILSEQ;
    }
    
    return -1;
}


// packing integeral number
#define _par_pack_bitint(p,bit,v,f) do { \
    par_type_t *pval = _par_pack_slice( p, PAR_TYPE##bit##_SIZE ); \
    pval->data.endian = p->endian; \
    pval->data.flag = (uint_fast8_t)f; \
    pval->data.kind = PAR_K_I##bit; \
    *((uint_fast##bit##_t*)(pval+PAR_TYPE_SIZE)) = (uint_fast##bit##_t)v; \
}while(0)


// positive integer
static inline int par_pack_uint( par_pack_t *p, uint_fast64_t num )
{
    if( num <= UINT8_MAX ){
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
static inline int par_pack_int( par_pack_t *p, int_fast64_t num )
{
    if( num >= INT8_MIN ){
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


// packing floating-point number
#define _par_pack_bitfloat(p,bit,v,f) do { \
    par_type_t *pval = _par_pack_slice( p, PAR_TYPE##bit##_SIZE ); \
    pval->data.endian = p->endian; \
    pval->data.flag = (uint_fast8_t)f; \
    pval->data.kind = PAR_K_F##bit; \
    *((par_float##bit##_t*)(pval+PAR_TYPE_SIZE)) = (par_float##bit##_t)(v); \
}while(0)


static inline int par_pack_float( par_pack_t *p, double num )
{
    _par_pack_bitfloat( p, 64, num, signbit( num ) );
    return 0;
}


// MARK: unpacking
// bin data
typedef struct {
    uint_fast8_t endian;
    size_t cur;
    size_t blksize;
    void *mem;
} par_unpack_t;


static inline void par_unpack_init( par_unpack_t *p, void *mem, size_t blksize )
{
    static const int endian = 1;
    
    // 0: little-endian, 1: big-endian
    p->endian = !(*(char*)&endian);
    p->cur = 0;
    p->mem = mem;
    p->blksize = blksize;
}


// swap byteorder
#define _par_bswap8(v)

#define _par_bswap16(v) do { \
    v = ((((v)>>8)&0x00FF)|(((v)<<8)&0xFF00)); \
}while(0)

#define _par_bswap32(v) do { \
    v = ((((v)>>24)&0x000000FF)|(((v)>>8)&0x0000FF00) | \
         (((v)<<8)&0x00FF0000)|(((v)<<24)&0xFF000000)); \
}while(0)

#define _par_bswap64(v) do { \
    v = ((((v)>>56)&0x00000000000000FF)|(((v)>>40)&0x000000000000FF00) | \
         (((v)>>24)&0x0000000000FF0000)|(((v)>>8)&0x00000000FF000000) | \
         (((v)<<8)&0x000000FF00000000)|(((v)<<24)&0x0000FF0000000000) | \
         (((v)<<40)&0x00FF000000000000)|(((v)<<56)&0xFF00000000000000)); \
}while(0)


// len: *(uint_fast[8-64]_t*)(mem + cur + PAR_TYPE_SIZE)
// val: mem + cur + PAR_TYPE[8-64]_SIZE
#define _par_unpack_vstr(ext,type,bit,e) do { \
    ext->len = *(uint_fast##bit##_t*)(type+PAR_TYPE_SIZE); \
    ext->val.str = (char*)(type+PAR_TYPE##bit##_SIZE); \
    if( bit > 8 && e != ext->endian ){ \
        _par_bswap##bit( ext->len ); \
    } \
}while(0)


#define _par_unpack_vint(ext,type,bit,e) do { \
    ext->val.u##bit = *(uint_fast##bit##_t*)(type+PAR_TYPE_SIZE); \
    if( bit > 8 && e != ext->endian ){ \
        _par_bswap##bit( ext->val.u##bit ); \
    } \
}while(0)


#define _par_unpack_vfloat(ext,type,bit,e) do { \
    ext->val.f##bit = *(par_float##bit##_t*)(type+PAR_TYPE_SIZE); \
    if( e != ext->endian ){ \
        _par_bswap##bit( ext->val.u##bit ); \
    } \
}while(0)


static inline int par_unpack( par_unpack_t *p, par_extract_t *ext )
{
    if( p->cur < p->blksize )
    {
        par_type_t *type = (par_type_t*)p->mem + p->cur;
        
        // set endian, kind and flag value
        ext->kind = type->data.kind;
        ext->flag = type->data.flag;
        ext->endian = type->data.endian;
        
        switch( ext->kind ){
            // 1 byte types
            case PAR_K_NIL ... PAR_K_I0:
                p->cur += PAR_TYPE_SIZE;
            break;
            case PAR_K_STR:
                _par_check_blkspc( p->blksize, p->cur, 
                                   _par_fsize2byte( ext->flag ) );
                switch( ext->flag ){
                    case PAR_F_SIZE8:
                        _par_unpack_vstr( ext, type, 8, p->endian );
                        p->cur += PAR_TYPE8_SIZE + ext->len;
                    break;
                    case PAR_F_SIZE16:
                        _par_unpack_vstr( ext, type, 16, p->endian );
                        p->cur += PAR_TYPE16_SIZE + ext->len;
                    break;
                    case PAR_F_SIZE32:
                        _par_unpack_vstr( ext, type, 32, p->endian );
                        p->cur += PAR_TYPE32_SIZE + ext->len;
                    break;
                    case PAR_F_SIZE64:
                        _par_unpack_vstr( ext, type, 64, p->endian );
                        p->cur += PAR_TYPE64_SIZE + ext->len;
                    break;
                    // illegal byte sequence
                    default:
                        errno = PARCEL_EILSEQ;
                        return -1;
                }
            break;
            case PAR_K_ARR ... PAR_K_MAP:
                if( type->data.flag == PAR_F_FIXLEN )
                {
                    _par_check_blkspc( p->blksize, p->cur, PAR_TYPE_SIZE );
                    ext->len = *((par_typelen_t*)(type+PAR_TYPE_SIZE));
                    // swap byteorder
                    if( p->endian != ext->endian ){
                        _par_bswap64( ext->len );
                    }
                    p->cur += PAR_TYPEX_SIZE;
                }
                else {
                    ext->len = 0;
                    p->cur += PAR_TYPE_SIZE;
                }
            break;
            // number
            // endian = 1:big-endian, 0:littel-endian
            // flag = 1:sign, 0:unsign
            case PAR_K_I8:
                _par_check_blkspc( p->blksize, p->cur, PAR_TYPE8_SIZE );
                _par_unpack_vint( ext, type, 8, p->endian );
                p->cur += PAR_TYPE8_SIZE;
            break;
            case PAR_K_I16:
                _par_check_blkspc( p->blksize, p->cur, PAR_TYPE16_SIZE );
                _par_unpack_vint( ext, type, 16, p->endian );
                p->cur += PAR_TYPE16_SIZE;
            break;
            case PAR_K_I32:
                _par_check_blkspc( p->blksize, p->cur, PAR_TYPE32_SIZE );
                _par_unpack_vint( ext, type, 32, p->endian );
                p->cur += PAR_TYPE32_SIZE;
            break;
            case PAR_K_I64:
                _par_check_blkspc( p->blksize, p->cur, PAR_TYPE64_SIZE );
                _par_unpack_vint( ext, type, 64, p->endian );
                p->cur += PAR_TYPE64_SIZE;
            break;
            case PAR_K_F64:
                _par_check_blkspc( p->blksize, p->cur, PAR_TYPE64_SIZE );
                _par_unpack_vfloat( ext, type, 64, p->endian );
                p->cur += PAR_TYPE64_SIZE;
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


#endif
