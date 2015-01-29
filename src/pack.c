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

#include "lparcel.h"

#define MODULE_MT       "parcel.pack_tbl"
#define MODULE_OO_MT    "parcel.pack"

#define LUANUM_ISDBL(val)   ((lua_Number)((lua_Integer)val) != val)

#define LUANUM_ISUINT(val)  (!signbit( val ) && !LUANUM_ISDBL( val ))

enum {
    PACK_TBL_INVAL = -1,
    PACK_TBL_EMPTY = 0,
    PACK_TBL_NITEM,
    PACK_TBL_NKEYS
};


typedef struct {
    lua_State *L;
    par_pack_t p;
} lpack_t;


static int pack_val( par_pack_t *p, lua_State *L, int idx );


static int pack_number( par_pack_t *p, lua_State *L, int idx )
{
    double num = lua_tonumber( L, idx );
    
    // set nan
    if( isnan( num ) ){
        return par_pack_nan( p );
    }
    // set inf
    else if( isinf( num ) ){
        return par_pack_inf( p, num );
    }
    // set zero
    else if( !num ){
        return par_pack_zero( p );
    }
    // float
    else if( LUANUM_ISDBL( num ) ){
        return par_pack_float64( p, num );
    }
    // signed integer
    else if( signbit( num ) ){
        return par_pack_int( p, (int_fast64_t)num );
    }
    
    // unsigned integer
    return par_pack_uint( p, (uint_fast64_t)num );
}


static int pack_map( par_pack_t *p, lua_State *L, size_t len )
{
    // append array
    if( par_pack_map( p, len ) == 0 )
    {
        // push space
        lua_pushnil( L );
        while( lua_next( L, -2 ) )
        {
            // append value
            if( pack_val( p, L, -2 ) != 0 ||
                pack_val( p, L, -1 ) != 0 ){
                lua_pop( L, 2 );
                return -1;
            }
            lua_pop( L, 1 );
        }
        return 0;
    }
    
    return -1;
}


static int pack_array( par_pack_t *p, lua_State *L, size_t len )
{
    // append array
    if( par_pack_array( p, len ) == 0 )
    {
        lua_Integer seq = 1;
        lua_Integer idx = 0;
        
        // push space
        lua_pushnil( L );
        while( lua_next( L, -2 ) )
        {
            idx = lua_tointeger( L, -2 );
            if( idx == seq ){
                seq++;
            }
            // append index
            else if( par_pack_idx( p, (uint_fast64_t)lua_tointeger( L, -2 ) ) != 0 ){
                lua_pop( L, 2 );
                return -1;
            }
            
            // append value
            if( pack_val( p, L, -1 ) != 0 ){
                lua_pop( L, 2 );
                return -1;
            }
            lua_pop( L, 1 );
        }
        return 0;
    }
    
    return -1;
}


// count
static int get_tbllen( lua_State *L, size_t *len )
{
    size_t nelts = 0;
    
    // push space
    lua_pushnil( L );
    // empty
    if( !lua_next( L, -2 ) ){
        return PACK_TBL_EMPTY;
    }
    
    // count number of array index
    do
    {
        // invalid array index value
        if( lua_type( L, -2 ) != LUA_TNUMBER || 
            !LUANUM_ISUINT( lua_tonumber( L, -2 ) ) ){
            goto CHECK_KEYTYPE;
        }
        nelts++;
        lua_pop( L, 1 );
    } while( lua_next( L, -2 ) );
    
    *len = nelts;
    
    return PACK_TBL_NITEM;


    // count number of map keys
    while( lua_next( L, -2 ) )
    {
CHECK_KEYTYPE:
        // check key type
        switch( lua_type( L, -2 ) ){
            case LUA_TNUMBER:
                // unsupported key type
                if( LUANUM_ISDBL( lua_tonumber( L, -2 ) ) ){
                    goto INVALID_KEY;
                }
            case LUA_TSTRING:
                lua_pop( L, 1 );
                nelts++;
            break;
            
            // unsupported key type
            default:
INVALID_KEY:
                lua_pop( L, 2 );
                return PACK_TBL_INVAL;
        }
    }
    
    *len = nelts;
    
    return PACK_TBL_NKEYS;
}


static int pack_val( par_pack_t *p, lua_State *L, int idx )
{
    const char *str = NULL;
    size_t len = 0;
    
    switch( lua_type( L, idx ) )
    {
        case LUA_TSTRING:
            str = lua_tolstring( L, idx, &len );
            return par_pack_str( p, (void*)str, len );
        
        case LUA_TBOOLEAN:
            return par_pack_bool( p, (uint8_t)lua_toboolean( L, idx ) );
        
        case LUA_TNUMBER:
            return pack_number( p, L, idx );
        
        case LUA_TTABLE:
            switch( get_tbllen( L, &len ) ){
                case PACK_TBL_EMPTY:
                    return par_pack_map( p, 0 );
                
                case PACK_TBL_NITEM:
                    return pack_array( p, L, len );
                
                case PACK_TBL_NKEYS:
                    return pack_map( p, L, len );
                
                // unsupported key type
                //case PACK_TBL_INVAL:
                default:
                    return -1;
            }
        
        //case LUA_TLIGHTUSERDATA:
        //case LUA_TFUNCTION:
        //case LUA_TUSERDATA:
        //case LUA_TTHREAD:
        //case LUA_TNONE:
        //case LUA_TNIL:
        default:
            return par_pack_nil( p );
    }
}


static int pack_lua( lua_State *L )
{
    // memory block size
    lua_Integer blksize = luaL_optinteger( L, 3, 0 );
    par_pack_t p;
    
    // check blksize
    if( blksize < 0 ){
        blksize = 0;
    }
    
    if( par_pack_init( &p, (size_t)blksize ) == 0 )
    {
        lua_settop( L, 2 );
        if( pack_val( &p, L, 2 ) == 0 ){
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
    lpack_t *l = luaL_checkudata( L, 1, MODULE_OO_MT );
    // pack value
    int rc = pack_val( &l->p, L, 2 );
    
    if( rc == 0 ){
        lua_settop( L, 0 );
        lua_pushlstring( L, l->p.mem, l->p.cur );
        // reset cursor
        l->p.cur = 0;
        return 1;
    }
    
    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );
    
    return 2;
}


static int tostring_lua( lua_State *L )
{
    return lparcel_tostring( L, MODULE_OO_MT );
}


static int gc_lua( lua_State *L )
{
    lpack_t *l = lua_touserdata( L, 1 );
    
    par_pack_dispose( &l->p );
    
    return 0;
}


static int alloc_lua( lua_State *L )
{
    // memory block size
    lua_Integer blksize = luaL_optinteger( L, 1, 0 );
    lpack_t *l = lua_newuserdata( L, sizeof( lpack_t ) );
    
    // check blksize
    if( blksize < 0 ){
        blksize = 0;
    }
    
    // alloc
    if( l && par_pack_init( &l->p, (size_t)blksize ) == 0 ){
        // retain references
        luaL_getmetatable( L, MODULE_OO_MT );
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
    struct luaL_Reg mmethod[] = {
        { "__call", pack_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        { "new", alloc_lua },
        { NULL, NULL }
    };
    // oo interface
    struct luaL_Reg oomethod[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { "__call", call_lua },
        { NULL, NULL }
    };
    
    // create metatable
    parcel_define_mt( L, MODULE_MT, mmethod, method );
    parcel_define_mt( L, MODULE_OO_MT, oomethod, NULL );
    // create module table
    lua_newtable( L );
    luaL_getmetatable( L, MODULE_MT );
    lua_setmetatable( L, -2 );
    
    return 1;
}
