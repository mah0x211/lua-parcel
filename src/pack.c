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
 *  pack.c
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/01/28.
 *
 */

#include "lparcel_pack.h"

#define MODULE_MT   "parcel.pack"



static int pack_lua( lua_State *L )
{
    par_pack_t p;

    if( par_pack_init( &p, 0, NULL, NULL ) == 0 )
    {
        lua_settop( L, 1 );
        if( lparcel_pack_val( &p, L, 1 ) == 0 ){
            lua_settop( L, 0 );
            lua_pushlstring( L, p.mem, p.cur );
            par_pack_dispose( &p );
            return 1;
        }
        par_pack_dispose( &p );
    }

    // got error
    lua_settop( L, 0 );
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


static int call_lua( lua_State *L )
{
    par_pack_t *p = luaL_checkudata( L, 1, MODULE_MT );
    // pack value
    int rc = 0;

    lua_settop( L, 2 );
    if( ( rc = lparcel_pack_val( p, L, 2 ) ) )
    {
        lua_settop( L, 0 );
        lua_pushlstring( L, p->mem, p->cur );
        // reset pack
        if( par_pack_reset( p ) == 0 ){
            return 1;
        }
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
    par_pack_t *p = lua_touserdata( L, 1 );

    par_pack_dispose( p );

    return 0;
}


static int alloc_lua( lua_State *L )
{
    // memory block size
    lua_Integer blksize = luaL_optinteger( L, 1, 0 );
    par_pack_t *p = lua_newuserdata( L, sizeof( par_pack_t ) );

    // check blksize
    if( blksize < 0 ){
        blksize = 0;
    }

    // alloc
    if( p && par_pack_init( p, (size_t)blksize, NULL, NULL ) == 0 ){
        // retain references
        luaL_getmetatable( L, MODULE_MT );
        lua_setmetatable( L, -2 );
        return 1;
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


LUALIB_API int luaopen_parcel_pack( lua_State *L )
{
    struct luaL_Reg funcs[] = {
        { "new", alloc_lua },
        { "pack", pack_lua },
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

