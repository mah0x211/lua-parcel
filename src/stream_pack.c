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
 *  stream_pack.c
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/02/01.
 *
 */

#include "lparcel_pack.h"

#define MODULE_MT   "parcel.stream.pack"

typedef struct {
    par_pack_t p;
    lua_State *L;
    lua_State *co;
    int ref_co;
    int ref_fn;
    const char *errstr;
} lparcel_fnstream_t;


static int coreduce( void *mem, size_t bytes, void *udata )
{
    lparcel_fnstream_t *fns = (lparcel_fnstream_t*)udata;
    int rc = 0;
    
    lstate_pushref( fns->co, fns->ref_fn );
    lua_pushinteger( fns->co, (lua_Integer)bytes );
    //lua_pushlightuserdata( fns->co, mem );
    lua_pushlstring( fns->co, mem, bytes );
    
    // run coroutine
#if LUA_VERSION_NUM >= 502
    rc = lua_resume( fns->co, fns->L, 2 );
#else
    rc = lua_resume( fns->co, 2 );
#endif

    switch( rc ){
        case LUA_YIELD:
            fns->errstr = "could not suspend";
        break;
        
        case LUA_ERRMEM:
        case LUA_ERRERR:
        case LUA_ERRSYNTAX:
        case LUA_ERRRUN:
            fns->errstr = lua_tostring( fns->co, -1 );
            printf("error: %s\n", fns->errstr);
        break;
    }
    lua_settop( fns->co, 0 );
    
    return -(!!rc);
}


static int call_lua( lua_State *L )
{
    lparcel_fnstream_t *fns = luaL_checkudata( L, 1, MODULE_MT );
    // pack value
    int rc = 0;
    
    lua_settop( L, 2 );
    if( ( rc = lparcel_pack_val( &fns->p, L, 2 ) ) == 0 )
    {
        lua_settop( L, 0 );
        if( ( rc = coreduce( fns->p.mem, fns->p.cur, (void*)fns ) ) == 0 ){
            lua_pushboolean( L, 1 );
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
    lparcel_fnstream_t *fns = lua_touserdata( L, 1 );
    
    lstate_unref( L, fns->ref_co );
    lstate_unref( L, fns->ref_fn );
    par_pack_dispose( &fns->p );
    
    return 0;
}


static int alloc_fnstream( lua_State *L, size_t blksize, int ref_fn )
{
    lparcel_fnstream_t *fns = lua_newuserdata( L, sizeof( lparcel_fnstream_t ) );
    
    // alloc
    if( fns && ( fns->co = lua_newthread( L ) ) && 
        par_pack_init( &fns->p, blksize, coreduce, (void*)fns ) == 0 ){
        fns->L = L;
        // retain refs
        fns->ref_co = lstate_ref( L );
        fns->ref_fn = ref_fn;
        // set metatable
        luaL_getmetatable( L, MODULE_MT );
        lua_setmetatable( L, -2 );
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}


static int alloc_lua( lua_State *L )
{
    // memory block size
    lua_Integer blksize = luaL_optinteger( L, 2, 0 );
    
    // check blksize
    if( blksize < 0 ){
        blksize = 0;
    }
    
    lua_settop( L, 1 );
    // check second argument
    switch( lua_type( L, 1 ) ){
        case LUA_TFUNCTION:
            return alloc_fnstream( L, (size_t)blksize, lstate_ref( L ) );
        break;
        //case LUA_TNUMBER:
        //    return alloc_streamfd( L, blksize );
        //break;
        
        default:
            return luaL_argerror( 
                L, 2, 
                "second argument must be function or writable descriptor" 
            );
    }
}


LUALIB_API int luaopen_parcel_stream_pack( lua_State *L )
{
    struct luaL_Reg funcs[] = {
        { "new", alloc_lua },
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

