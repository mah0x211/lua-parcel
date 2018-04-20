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
 *  lparcel.h
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/01/28.
 *
 */

#ifndef ___LUA_PARCEL_H___
#define ___LUA_PARCEL_H___

#include "parcel.h"
#include <lua.h>
#include <lauxlib.h>


// helper macros
#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)

#define lstate_ref(L) \
    (luaL_ref( L, LUA_REGISTRYINDEX ))

#define lstate_pushref(L,ref) \
    lua_rawgeti( L, LUA_REGISTRYINDEX, ref )

#define lstate_unref(L,ref) \
    luaL_unref( L, LUA_REGISTRYINDEX, ref )


#define LUANUM_ISDBL(val)   ((lua_Number)((lua_Integer)val) != val)

#define LUANUM_ISUINT(val)  (!signbit( val ) && !LUANUM_ISDBL( val ))


// prototypes
LUALIB_API int luaopen_parcel_pack( lua_State *L );
LUALIB_API int luaopen_parcel_unpack( lua_State *L );


// common metamethods
#define lparcel_tostring(L,tname) ({ \
    lua_pushfstring( L, tname ": %p", lua_touserdata( L, 1 ) ); \
    1; \
})


// metanames
// module definition register
static inline int lparcel_define_method( lua_State *L,
                                         struct luaL_Reg method[] )
{
    struct luaL_Reg *ptr = method;

    // methods
    lua_newtable( L );
    do {
        lstate_fn2tbl( L, ptr->name, ptr->func );
        ptr++;
    } while( ptr->name );

    return 1;
}


static inline int lparcel_define_mt( lua_State *L, const char *tname,
                                     struct luaL_Reg mmethod[],
                                     struct luaL_Reg method[] )
{
    // create table __metatable
    if( luaL_newmetatable( L, tname ) )
    {
        struct luaL_Reg *ptr = mmethod;

        // metamethods
        do {
            lstate_fn2tbl( L, ptr->name, ptr->func );
            ptr++;
        } while( ptr->name );

        // methods
        if( method ){
            lua_pushstring( L, "__index" );
            lparcel_define_method( L, method );
            lua_rawset( L, -3 );
        }
        lua_pop( L, 1 );

        return 1;
    }

    return 0;
}




#endif
