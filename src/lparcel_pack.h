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
 *  lparcel_pack.h
 *  lua-parcel
 *
 *  Created by Masatoshi Teruya on 2015/02/03.
 *
 */

#ifndef ___LUA_PARCEL_PACK_H___
#define ___LUA_PARCEL_PACK_H___

#include "lparcel.h"

#define LUANUM_ISDBL(val)   ((lua_Number)((lua_Integer)val) != val)

#define LUANUM_ISUINT(val)  (!signbit( val ) && !LUANUM_ISDBL( val ))

// check length of table as array
enum {
    LP_TBL_NELTS_INVAL = -1,
    LP_TBL_NELTS_EMPTY = 0,
    LP_TBL_NELTS_ARRAY,
    LP_TBL_NELTS_MAP
};

static inline int lparcel_tblnelts( lua_State *L, size_t *len )
{
    size_t nelts = 0;
    
    // push space
    lua_pushnil( L );
    // empty
    if( !lua_next( L, -2 ) ){
        return LP_TBL_NELTS_EMPTY;
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
    
    return LP_TBL_NELTS_ARRAY;


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
                return LP_TBL_NELTS_INVAL;
        }
    }
    
    *len = nelts;
    
    return LP_TBL_NELTS_MAP;
}



static inline int lparcel_pack_val( par_pack_t *p, lua_State *L, int idx );

static inline int lparcel_pack_map( par_pack_t *p, lua_State *L, size_t len )
{
    // append map
    if( par_pack_map( p, len ) == 0 )
    {
        // push space
        lua_pushnil( L );
        while( lua_next( L, -2 ) )
        {
            // append value
            if( lparcel_pack_val( p, L, -2 ) != 0 ||
                lparcel_pack_val( p, L, -1 ) != 0 ){
                lua_pop( L, 2 );
                return -1;
            }
            lua_pop( L, 1 );
        }
        return 0;
    }
    
    return -1;
}


static inline int lparcel_pack_array( par_pack_t *p, lua_State *L, size_t len )
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
            if( lparcel_pack_val( p, L, -1 ) != 0 ){
                lua_pop( L, 2 );
                return -1;
            }
            lua_pop( L, 1 );
        }
        return 0;
    }
    
    return -1;
}


static inline int lparcel_pack_number( par_pack_t *p, lua_State *L, int idx )
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


static inline int lparcel_pack_val( par_pack_t *p, lua_State *L, int idx )
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
            return lparcel_pack_number( p, L, idx );
        
        case LUA_TTABLE:
            switch( lparcel_tblnelts( L, &len ) ){
                case LP_TBL_NELTS_EMPTY:
                    return par_pack_map( p, 0 );
                
                case LP_TBL_NELTS_ARRAY:
                    return lparcel_pack_array( p, L, len );
                
                case LP_TBL_NELTS_MAP:
                    return lparcel_pack_map( p, L, len );
                
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


#endif
