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
 *  unpack.c
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/01/28.
 *
 */

#include "lparcel.h"

#define MODULE_MT   "parcel.unpack"

typedef struct {
    par_unpack_t p;
    int ref_mem;
} lunpack_t;


static int unpack_val( lua_State *L, par_unpack_t *p, par_extract_t *ext );
static int ext2lua( lua_State *L, par_unpack_t *p, par_extract_t *ext );


static int unpack_array_val( lua_State *L, par_unpack_t *p, par_extract_t *ext,
                             int *idx )
{
    int rc = unpack_val( L, p, ext );

    switch( rc ){
        case 0:
            lua_rawseti( L, -2, (*idx)++ );
        break;

        // non-consecutive array
        // unpack idx-value pair
        case PAR_ISA_IDX:
            // unpack key-value pair
            // unpack key
            if( ( rc = par_unpack_idx( p, ext ) ) == 0 &&
                ( rc = ext2lua( L, p, ext ) ) == 0 &&
                ( rc = unpack_val( L, p, ext ) ) == 0 ){
                lua_rawset( L, -3 );
            }
        break;
    }

    return rc;
}


static int unpack_array( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    int rc = 0;
    int idx = 1;
    size_t len = ext->size.len;
    size_t i = 0;

    // create table for array
    lua_createtable( L, (int)len, 0 );

    // unpack array items
    for(; i < len && rc == 0; i++ ){
        rc = unpack_array_val( L, p, ext, &idx );
    }

    return rc;
}


static int unpack_sarray( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    int rc = 0;
    int idx = 1;

    // create table for array
    lua_createtable( L, 0, 0 );

UNPACK_SARR:
    // unpack array items
    rc = unpack_array_val( L, p, ext, &idx );
    switch( rc ){
        case 0:
            goto UNPACK_SARR;

        // end-of-stream
        case PAR_ISA_EOS:
            return 0;
    }

    // got error
    return rc;
}



// unpack key of hashmap
static int unpack_map_val( lua_State *L, par_unpack_t *p, par_extract_t *ext,
                           int allow_eos )
{
    int rc = par_unpack_key( p, ext, allow_eos );

    // unpack key
    if( rc == 0 &&
        ( rc = ext2lua( L, p, ext ) ) == 0 &&
        ( rc = unpack_val( L, p, ext ) ) == 0 ){
        lua_rawset( L, -3 );
    }

    return rc;
}


static int unpack_map( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    int rc = 0;
    size_t i = 0;
    size_t len = ext->size.len;

    // create table for hashmap
    lua_createtable( L, 0, (int)len );

    // unpack key-value pair
    for(; i < len && rc == 0; i++ ){
        rc = unpack_map_val( L, p, ext, 0 );
    }

    return rc;
}


static int unpack_smap( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    int rc = 0;
    // create table for hashmap
    lua_createtable( L, 0, 0 );

UNPACK_SMAP:
    // unpack key-value pair
    rc = unpack_map_val( L, p, ext, 1 );
    switch( rc ){
        case 0:
            goto UNPACK_SMAP;

        // end-of-stream
        case PAR_ISA_EOS:
            return 0;
    }

    return rc;
}


static int unpack_set( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    int rc = 0;
    size_t len = ext->size.len;
    size_t i = 0;

    // create table for set
    lua_createtable( L, (int)len, 0 );
    // unpack set items
    for(; i < len && rc == 0; i++ )
    {
        if( ( rc = par_unpack_elm( p, ext, 0 ) ) == 0 ){
            rc = ext2lua( L, p, ext );
        }
    }

    return rc;
}


static int unpack_sset( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    int rc = 0;

    // create table for set
    lua_createtable( L, 0, 0 );

UNPACK_SSET:
    // unpack element
    if( ( rc = par_unpack_elm( p, ext, 1 ) ) == 0 )
    {
        rc = ext2lua( L, p, ext );
        switch( rc ){
            case 0:
                goto UNPACK_SSET;

            // end-of-stream
            case PAR_ISA_EOS:
                return 0;
        }
    }

    // got error
    return rc;
}


static int ext2lua( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    switch( ext->isa )
    {
        // nil
        case PAR_ISA_NIL:
            lua_pushnil( L );
            return 0;

        // boolean
        case PAR_ISA_TRUE:
            lua_pushboolean( L, 1 );
            return 0;
        case PAR_ISA_FALSE:
            lua_pushboolean( L, 0 );
            return 0;

        // nan
        case PAR_ISA_NAN:
            lua_pushnumber( L, NAN );
            return 0;

        // inf
        case PAR_ISA_PINF:
            lua_pushnumber( L, INFINITY );
            return 0;
        case PAR_ISA_NINF:
            lua_pushnumber( L, -INFINITY );
            return 0;

        // 8 byte pack
        // string
        case PAR_ISA_RAW8 ... PAR_ISA_STR64:
        case PAR_ISA_STR5:
            lua_pushlstring( L, ext->val.bytea, ext->size.len );
            return 0;

        // signed values
        #define lstate_push_extint( L, ext, bit ) do { \
            lua_pushinteger( L, (lua_Integer)ext->val.i##bit ); \
        }while(0)
        case PAR_ISA_S6:
        case PAR_ISA_S6N:
        case PAR_ISA_S8:
            lstate_push_extint( L, ext, 8 );
            return 0;
        case PAR_ISA_S16:
            lstate_push_extint( L, ext, 16 );
            return 0;
        case PAR_ISA_S32:
            lstate_push_extint( L, ext, 32 );
            return 0;
        case PAR_ISA_S64:
            lstate_push_extint( L, ext, 64 );
            return 0;
        #undef lstate_push_extint

        // unsigned values
        #define lstate_push_extuint( L, ext, bit ) do { \
            lua_pushinteger( L, (lua_Integer)ext->val.u##bit ); \
        }while(0)
        case PAR_ISA_U8:
            lstate_push_extuint( L, ext, 8 );
            return 0;
        case PAR_ISA_U16:
            lstate_push_extuint( L, ext, 16 );
            return 0;
        case PAR_ISA_U32:
            lstate_push_extuint( L, ext, 32 );
            return 0;
        case PAR_ISA_U64:
            lstate_push_extuint( L, ext, 64 );
            return 0;
        #undef lstate_push_extuint

        // floating-point values
        case PAR_ISA_F32:
            //printf("float32: %f | %f - %f\n", ext.val.f32, FLT_MIN, FLT_MAX);
            lua_pushnumber( L, ext->val.f32 );
            return 0;
        case PAR_ISA_F64:
            //printf("float64: %f\n", ext.val.f64);
            lua_pushnumber( L, ext->val.f64 );
            return 0;

        // array
        case PAR_ISA_ARR4:
        case PAR_ISA_ARR8 ... PAR_ISA_ARR64:
            return unpack_array( L, p, ext );

        // map
        case PAR_ISA_MAP4:
        case PAR_ISA_MAP8 ... PAR_ISA_MAP64:
            return unpack_map( L, p, ext );

        // set
        case PAR_ISA_SET8 ... PAR_ISA_SET64:
            return unpack_set( L, p, ext );

        // stream array/map/set
        case PAR_ISA_SARR:
            return unpack_sarray( L, p, ext );

        case PAR_ISA_SMAP:
            return unpack_smap( L, p, ext );

        case PAR_ISA_SSET:
            return unpack_sset( L, p, ext );

        // array index
        case PAR_ISA_IDX:
        // end-of-stream
        case PAR_ISA_EOS:
            return ext->isa;

        // unknown type
        default:
            return -1;
    }
}


static int unpack_val( lua_State *L, par_unpack_t *p, par_extract_t *ext )
{
    switch( par_unpack( p, ext ) )
    {
        case -2:
            return -2;

        case 0:
            return ext2lua( L, p, ext );

        default:
            return -1;
    }
}


static int unpack_lua( lua_State *L )
{
    size_t len = 0;
    const char *mem = (const char*)luaL_checklstring( L, 1, &len );
    par_unpack_t p;
    par_extract_t ext;

    // init
    par_unpack_init( &p, (void*)mem, len );
    lua_settop( L, 1 );
    // unpack
    if( unpack_val( L, &p, &ext ) == 0 ){
        return lua_gettop( L ) - 1;
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


static int call_lua( lua_State *L )
{
    lunpack_t *lu = luaL_checkudata( L, 1, MODULE_MT );
    par_extract_t ext;

    // unpack
    if( unpack_val( L, &lu->p, &ext ) != -1 ){
        return lua_gettop( L ) - 1;
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


static int tostring_lua( lua_State *L )
{
    return lparcel_tostring( L, MODULE_MT );
}


static int gc_lua( lua_State *L )
{
    lunpack_t *lu = lua_touserdata( L, 1 );

    lstate_unref( L, lu->ref_mem );

    return 0;
}


static int new_lua( lua_State *L )
{
    size_t len = 0;
    const char *mem = (const char*)luaL_checklstring( L, 1, &len );
    int ref = 0;
    lunpack_t *lu = NULL;

    lua_settop( L, 1 );
    ref = lstate_ref( L );
    lu = lua_newuserdata( L, sizeof( lunpack_t ) );
    lu->ref_mem = ref;
    par_unpack_init( &lu->p, (void*)mem, len );
    luaL_getmetatable( L, MODULE_MT );
    lua_setmetatable( L, -2 );

    return 1;
}


LUALIB_API int luaopen_parcel_unpack( lua_State *L )
{
    struct luaL_Reg funcs[] = {
        { "new", new_lua },
        { "unpack", unpack_lua },
        { NULL, NULL }
    };
    // oo interface
    struct luaL_Reg mmethod[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { "__call", call_lua },
        { NULL, NULL }
    };

    // create metatable
    lparcel_define_mt( L, MODULE_MT, mmethod, NULL );
    // create module table
    lparcel_define_method( L, funcs );

    return 1;
}

