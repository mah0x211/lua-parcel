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
    -----------------
    0               1
    0 1 2 3 4 5 6 7 0
    +---------------+
    type    |info   |
    +---------------+
*/

// type: 0-15
enum {
    // 1 byte types
    PAR_K_NIL = 0,
    // flag = 1:true, 0:false
    PAR_K_BOL,
    PAR_K_TBL,
    PAR_K_I0,
    
    // 8 byte types
    PAR_K_STR,
    PAR_K_ARR,
    PAR_K_MAP,
    
    // 2-9 byte
    // endian = 1:big-endian, 0:littel-endian
    // flag = 1:sign, 0:unsign
    PAR_K_I8,
    PAR_K_I16,
    PAR_K_I32,
    PAR_K_I64,
    PAR_K_F64
};

// endianness
#define PAR_A_LITEND    0x0
#define PAR_A_BIGEND    0x1

// signedness
#define PAR_A_UNSIGN    0x0
#define PAR_A_SIGNED    0x1

#define PAR_INFO_FIELDS \
    uint_fast8_t endian:1; \
    uint_fast8_t flag:1; \
    uint_fast8_t kind:6


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
        } val;
    } data;
} par_extract_t;

#undef PAR_INFO_FIELDS


// FLOAT 32 BIT
typedef par_type32_t par_f32_t;

// FLOAT 64 BIT
typedef par_type64_t par_f64_t;


// MARK: parcel memory

#define ALLOC_UNIT  1024
#define NALLOC_MAX  SIZE_MAX/ALLOC_UNIT

// bin data
typedef struct {
    size_t cur;
    size_t nalloc;
    size_t total;
    void *mem;
} parcel_t;


static inline int par_pack_init( parcel_t *p )
{
    if( ( p->mem = malloc(0) ) ){
        p->cur = 0;
        p->nalloc = 0;
        p->total = 0;
        return 0;
    }
    else {
        errno = ENOMEM;
    }

    return -1;
}


static inline void par_pack_dispose( parcel_t *p )
{
    if( p->mem ){
        free( p->mem );
        p->mem = NULL;
    }
}


static inline int par_pack_increase( parcel_t *p, size_t bytes )
{
    size_t remain = p->total - p->cur;

    // remain < bytes
    if( remain < bytes )
    {
        size_t nalloc;
        
        bytes -= remain;
        nalloc = ( bytes / ALLOC_UNIT ) + 
                 ( bytes % ALLOC_UNIT ? 1 : 0 ) + 
                 p->nalloc;
        
        if( nalloc < NALLOC_MAX )
        {
            void *buf;
            
            bytes = ALLOC_UNIT * nalloc;
            // overhead: doing memory copy in internally
            if( ( buf = realloc( p->mem, bytes ) ) ){
                p->mem = buf;
                p->nalloc = nalloc;
                p->total = bytes;
                //memset( buf + p->cur, 0, bytes - p->cur );
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


// MARK: packing

static inline void *par_pack_update_cur( parcel_t *p, size_t bytes )
{
    void *mem = p->mem + p->cur;
    
    p->cur += bytes;
    
    return mem;
}

// allocate sizeof(t) and extra bytes
#define par_pack_slice_ex(p,t,ex) ({ \
    if( par_pack_increase( p, sizeof(t) + ex ) == -1 ){ \
        return -1; \
    } \
    (t*)par_pack_update_cur( p, sizeof(t) + ex ); \
})


// allocate sizeof(t) bytes
#define par_pack_slice(p,t)  par_pack_slice_ex(p,t,0)


static inline int par_pack_bool( parcel_t *p, int val )
{
    par_bol_t *pval = par_pack_slice( p, par_bol_t );
    
    pval->data.kind = PAR_K_BOL;
    pval->data.flag = !!val;
    
    return 0;
}


static inline int par_pack_nil( parcel_t *p )
{
    par_nil_t *pval = par_pack_slice( p, par_nil_t );
    
    pval->data.kind = PAR_K_NIL;
    
    return 0;
}


static inline int par_pack_tbl( parcel_t *p )
{
    par_tbl_t *pval = par_pack_slice( p, par_tbl_t );
    
    pval->data.kind = PAR_K_TBL;
    
    return 0;
}


static inline int par_pack_str( parcel_t *p, const char *val, size_t len )
{
    par_str_t *pval = par_pack_slice_ex( p, par_str_t, len );
    
    pval->data.kind = PAR_K_STR;
    pval->data.len = len;
    memcpy( (void*)(pval+1), val, len );
    
    return 0;
}


static inline int par_pack_arr( parcel_t *p, size_t *idx )
{
    par_arr_t *pval = NULL;
    
    *idx = p->cur;
    pval = par_pack_slice( p, par_arr_t );
    pval->data.kind = PAR_K_ARR;
    
    return 0;
}


static inline int par_pack_map( parcel_t *p, size_t *idx )
{
    par_map_t *pval = NULL;
    
    *idx = p->cur;
    pval = par_pack_slice( p, par_map_t );
    pval->data.kind = PAR_K_MAP;
    
    return 0;
}


static inline int par_pack_tbllen( parcel_t *p, size_t idx, size_t len )
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


static inline int par_pack_zero( parcel_t *p )
{
    par_type0_t *pval = par_pack_slice( p, par_type0_t );
    
    pval->data.kind = PAR_K_I0;
    
    return 0;
}


#define par_pack_bitint(p,bit,v,f) do { \
    par_type ## bit ## _t *pval = par_pack_slice( p, par_type ## bit ## _t ); \
    pval->data.kind = PAR_K_I##bit; \
    pval->data.flag = (uint_fast8_t)f; \
    *((uint_fast ## bit ## _t*)pval->data.val) = (uint_fast ## bit ## _t)v; \
}while(0)


// positive integer
static inline int par_pack_uint( parcel_t *p, uint_fast64_t num )
{
    if( num == 0 ){
        return par_pack_zero( p );
    }
    else if( num <= UINT8_MAX ){
        par_pack_bitint( p, 8, num, PAR_A_UNSIGN );
    }
    else if( num <= UINT16_MAX ){
        par_pack_bitint( p, 16, num, PAR_A_UNSIGN );
    }
    else if( num <= UINT32_MAX ){
        par_pack_bitint( p, 32, num, PAR_A_UNSIGN );
    }
    else {
        par_pack_bitint( p, 64, num, PAR_A_UNSIGN );
    }
    
    return 0;
}


// negative integer
static inline int par_pack_int( parcel_t *p, int_fast64_t num )
{
    if( num == 0 ){
        return par_pack_zero( p );
    }
    else if( num >= INT8_MIN ){
        par_pack_bitint( p, 8, num, PAR_A_SIGNED );
    }
    else if( num >= INT16_MIN ){
        par_pack_bitint( p, 16, num, PAR_A_SIGNED );
    }
    else if( num >= INT32_MIN ){
        par_pack_bitint( p, 32, num, PAR_A_SIGNED );
    }
    else {
        par_pack_bitint( p, 64, num, PAR_A_SIGNED );
    }
    
    return 0;
}



// MARK: unpacking

#define par_unpack_bitint(cur,t,bit,ext) do { \
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


static inline int par_unpack( parcel_t *p, par_extract_t *ext )
{
    par_type_t *type = NULL;

    if( p->cur < p->total )
    {
        type = (par_type_t*)p->mem + p->cur;
        
        ext->data.kind = type->data.kind;
        // 1 byte types
        if( type->data.kind <= PAR_K_I0 )
        {
            // set boolean value
            // flag = 1:true, 0:false
            if( type->data.kind == PAR_K_BOL ){
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
            par_unpack_bitint( &p->cur, type, 8, ext );
        }
        else if( type->data.kind == PAR_K_I16 ){
            par_unpack_bitint( &p->cur, type, 16, ext );
        }
        else if( type->data.kind == PAR_K_I32 ){
            par_unpack_bitint( &p->cur, type, 32, ext );
        }
        else if( type->data.kind == PAR_K_I64 ){
            par_unpack_bitint( &p->cur, type, 64, ext );
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
