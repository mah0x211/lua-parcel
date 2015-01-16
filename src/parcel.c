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
 *  parcel.c
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/01/15.
 *
 */

#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include "parcel.h"


// helper macros
#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)


static int pack_val( parcel_t *b, lua_State *L, int idx );

static int pack_tbl( parcel_t *b, lua_State *L, int idx, int ktype, 
                     size_t vidx )
{
    size_t len = 0;
    
    // key: -2, val: -1
    do
    {
        // unsupported key
        if( lua_type( L, -2 ) != ktype ){
            errno = ENOTSUP;
            return -1;
        }
        // append key-val pair
        else if( pack_val( b, L, -2 ) != 0 ||
                 pack_val( b, L, -1 ) != 0 ){
            return -1;
        }
        len++;
        lua_pop( L, 1 );
    } while( lua_next( L, -2 ) );
    
    // set length
    return par_pack_tbllen( b, vidx, len );
}


static int pack_val( parcel_t *p, lua_State *L, int idx )
{
    const char *str = NULL;
    double num = 0;
    double inum = 0;
    size_t len = 0;
    size_t vidx = 0;
    int ktype = 0;
    
    switch( lua_type( L, idx ) )
    {
        case LUA_TLIGHTUSERDATA:
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
        case LUA_TNONE:
            errno = EINVAL;
            return -1;
        
        case LUA_TBOOLEAN:
            return par_pack_bool( p, lua_toboolean( L, idx ) );
        
        case LUA_TNUMBER:
            num = lua_tonumber( L, idx );
            // set nan
            if( isnan( num ) ){
                return par_pack_nan( p );
            }
            // set inf
            else if( isinf( num ) ){
                return par_pack_inf( p, num < 0 );
            }
            // set zero
            else if( num == 0.0 ){
                return par_pack_zero( p );
            }
            // set integer
            else if( modf( num, (double*)&inum ) == 0.0 )
            {
                // unsigned
                if( inum > 0.0 ){
                    return par_pack_uint( p, (uint_fast64_t)num );
                }
                
                // signed
                return par_pack_int( p, (int_fast64_t)num );
            }
            // set float
            return par_pack_float( p, num );
        
        case LUA_TSTRING:
            str = lua_tolstring( L, idx, &len );
            return par_pack_str( p, str, len );
        
        case LUA_TNIL:
            return par_pack_nil( p );
    }
    
    // push space
    lua_pushnil( L );
    // empty table
    if( !lua_next( L, -2 ) ){
        return par_pack_tbl( p );
    }
    
    // check types
    ktype = lua_type( L, -2 );
    switch( ktype ){
        case LUA_TNUMBER:
            if( par_pack_arr( p, &vidx ) != 0 ){
                return -1;
            }
        break;
        case LUA_TSTRING:
            if( par_pack_map( p, &vidx ) != 0 ){
                return -1;
            }
        break;
        
        // unsupported key
        default:
            errno = ENOTSUP;
            return -1;
    }
    
    return pack_tbl( p, L, idx, ktype, vidx );
}


static int pack_lua( lua_State *L )
{
    parcel_t p;
    
    if( par_pack_init( &p ) != 0 ){
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
        return 2;
    }
    // pack
    else if( pack_val( &p, L, 1 ) == 0 ){
        lua_pop( L, 1 );
        lua_pushlstring( L, p.mem, p.cur );
        par_pack_dispose( &p );
        return 1;
    }
    
    par_pack_dispose( &p );
    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}



static int unpack_val( lua_State *L, parcel_t *p );

static int unpack_tbl( lua_State *L, parcel_t *p, size_t len )
{
    // unpack key-value
    while( len-- )
    {
        if( unpack_val( L, p ) != 1 ||
            unpack_val( L, p ) != 1 ){
            return -1;
        }
        lua_rawset( L, -3 );
    }
    
    return 1;
}


static int unpack_val( lua_State *L, parcel_t *p )
{
    par_extract_t ext;
    // unpacking
    int rc = par_unpack( p, &ext );
    
    if( rc == 0 ){
        return 0;
    }
    else if( rc == -1 ){
        return -1;
    }
    
    switch( ext.data.kind )
    {
        // 1 byte pack
        // nil
        case PAR_K_NIL:
            lua_pushnil( L );
            return 1;
        // boolean
        // flag = 1:true, 0:false
        case PAR_K_BOL:
            lua_pushboolean( L, ext.data.flag );
            return 1;
        // nan
        case PAR_K_NAN:
            lua_pushnumber( L, NAN );
            return 1;
        // inf
        case PAR_K_INF:
            lua_pushnumber( L, ( ext.data.flag ) ? -INFINITY : INFINITY );
            return 1;
        // zero
        case PAR_K_I0:
            lua_pushinteger( L, 0 );
            return 1;
        // table
        case PAR_K_TBL:
            lua_newtable( L );
            return 1;
        
        // 8 byte pack
        // string
        case PAR_K_STR:
            lua_pushlstring( L, ext.data.val.str, ext.data.len );
            return 1;
        
        // 2-9 byte pack
        // number
        // endian = 1:big-endian, 0:littel-endian
        // flag = 1:sign, 0:unsign
        #define lstate_push_extint( L, ext, bit ) do { \
            if( ext.data.flag ){ \
                lua_pushinteger( L, (lua_Integer)ext.data.val.i##bit ); \
            } \
            else { \
                lua_pushinteger( L, (lua_Integer)ext.data.val.u##bit ); \
            } \
        }while(0)
        
        case PAR_K_I8:
            lstate_push_extint( L, ext, 8 );
            return 1;
        case PAR_K_I16:
            lstate_push_extint( L, ext, 16 );
            return 1;
        case PAR_K_I32:
            lstate_push_extint( L, ext, 32 );
            return 1;
        case PAR_K_I64:
            lstate_push_extint( L, ext, 64 );
            return 1;
        
        #undef lstate_push_extnum
        
        case PAR_K_F64:
            lua_pushnumber( L, ext.data.val.f64 );
            return 1;
        
        // array
        case PAR_K_ARR:
            lua_createtable( L, (int)ext.data.len, 0 );
        break;
        // map
        case PAR_K_MAP:
            lua_createtable( L, 0, (int)ext.data.len );
        break;
    }

    return unpack_tbl( L, p, ext.data.len );
}


static int unpack_lua( lua_State *L )
{
    size_t len = 0;
    const char *mem = (const char*)luaL_checklstring( L, 1, &len );
    parcel_t p = {
        .cur = 0,
        .total = len,
        .mem = (void*)mem,
    };
    
    // unpack
    if( unpack_val( L, &p ) != -1 ){
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}


LUALIB_API int luaopen_parcel( lua_State *L )
{
    struct luaL_Reg method[] = {
        { "pack", pack_lua },
        { "unpack", unpack_lua },
        { NULL, NULL }
    };
    struct luaL_Reg *ptr = method;

    // create table
    lua_newtable( L );
    // methods
    do {
        lstate_fn2tbl( L, ptr->name, ptr->func );
        ptr++;
    } while( ptr->name );
    
    return 1;
}

