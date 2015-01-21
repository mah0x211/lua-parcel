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



static int pack_val( par_pack_t *b, lua_State *L, int idx );

static int pack_tbl( par_pack_t *b, lua_State *L, int idx, int ktype, 
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
    return par_pack_arrmaplen( b, vidx, len );
}


static int pack_val( par_pack_t *p, lua_State *L, int idx )
{
    const char *str = NULL;
    double num = 0;
    double inum = 0;
    size_t len = 0;
    size_t vidx = 0;
    int ktype = 0;
    
    switch( lua_type( L, idx ) )
    {
        case LUA_TBOOLEAN:
            return par_pack_bool( p, (uint8_t)lua_toboolean( L, idx ) );
        
        case LUA_TNUMBER:
            num = lua_tonumber( L, idx );
            // set nan
            if( isnan( num ) ){
                return par_pack_nan( p );
            }
            // set inf
            else if( isinf( num ) ){
                return par_pack_inf( p, signbit( num ) );
            }
            // set zero
            else if( !num ){
                return par_pack_zero( p );
            }
            // set integer
            else if( modf( num, (double*)&inum ) == 0.0 )
            {
                // signed
                if( signbit( inum ) ){
                    return par_pack_int( p, (int_fast64_t)num );
                }
                // unsigned
                return par_pack_uint( p, (uint_fast64_t)num );
            }
            // set float
            // lua_Number == double
            return par_pack_float64( p, num );
        
        case LUA_TSTRING:
            str = lua_tolstring( L, idx, &len );
            return par_pack_str( p, (void*)str, len );
        
        case LUA_TNIL:
            return par_pack_nil( p );
        
        case LUA_TTABLE:
            break;
        
        //case LUA_TLIGHTUSERDATA:
        //case LUA_TFUNCTION:
        //case LUA_TUSERDATA:
        //case LUA_TTHREAD:
        //case LUA_TNONE:
        default:
            return par_pack_nil( p );
        
    }
    
    // push space
    lua_pushnil( L );
    // empty table
    if( !lua_next( L, -2 ) ){
        return par_pack_empty( p );
    }
    
    // check types
    ktype = lua_type( L, -2 );
    switch( ktype ){
        case LUA_TNUMBER:
            if( par_pack_arridx( p, PAR_A_BIT64, &vidx ) != 0 ){
                return -1;
            }
        break;
        case LUA_TSTRING:
            if( par_pack_mapidx( p, PAR_A_BIT64, &vidx ) != 0 ){
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
    if( lua_gettop( L ) )
    {
        // memory block size
        lua_Integer blksize = luaL_optinteger( L, 2, 0 );
        int rv = 0;
        par_pack_t p;
        
        // check blksize
        if( blksize < 0 ){
            blksize = 0;
        }
        // remove arguments wo first argument
        lua_settop( L, 1 );
        
        if( ( rv = par_pack_init( &p, (size_t)blksize ) ) == 0 )
        {
            // pack value
            if( ( rv = pack_val( &p, L, 1 ) ) == 0 ){
                lua_settop( L, 0 );
                lua_pushlstring( L, p.mem, p.cur );
            }
            par_pack_dispose( &p );
        }
        
        // got error
        if( rv != 0 ){
            lua_settop( L, 0 );
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            return 2;
        }
        
    }
    // no argument
    else {
        lua_pushnil( L );
    }
    
    return 1;
}




// stream


static int spack_val( par_spack_t *b, lua_State *L, int idx );

static int spack_tbl( par_spack_t *p, lua_State *L, int idx, int ktype )
{
    // key: -2, val: -1
    do
    {
        // unsupported key
        if( lua_type( L, -2 ) != ktype ){
            errno = ENOTSUP;
            return -1;
        }
        // append key-val pair
        else if( spack_val( p, L, -2 ) != 0 ||
                 spack_val( p, L, -1 ) != 0 ){
            return -1;
        }
        lua_pop( L, 1 );
    } while( lua_next( L, -2 ) );
    
    return par_spack_eos( p );
}


static int spack_val( par_spack_t *p, lua_State *L, int idx )
{
    const char *str = NULL;
    double num = 0;
    double inum = 0;
    size_t len = 0;
    int ktype = 0;
    
    switch( lua_type( L, idx ) )
    {
        case LUA_TBOOLEAN:
            return par_spack_bool( p, (uint8_t)lua_toboolean( L, idx ) );
        
        case LUA_TNUMBER:
            num = lua_tonumber( L, idx );
            // set nan
            if( isnan( num ) ){
                return par_spack_nan( p );
            }
            // set inf
            else if( isinf( num ) ){
                return par_spack_inf( p, signbit( num ) );
            }
            // set zero
            else if( !num ){
                return par_spack_zero( p );
            }
            // set integer
            else if( modf( num, (double*)&inum ) == 0.0 )
            {
                // signed
                if( signbit( inum ) ){
                    return par_spack_int( p, (int_fast64_t)num );
                }
                // unsigned
                return par_spack_uint( p, (uint_fast64_t)num );
            }
            // set float
            // lua_Number == double
            return par_spack_float64( p, num );
        
        case LUA_TSTRING:
            str = lua_tolstring( L, idx, &len );
            return par_spack_str( p, (void*)str, len );
        
        case LUA_TNIL:
            return par_spack_nil( p );
        
        case LUA_TTABLE:
            break;
        
        //case LUA_TLIGHTUSERDATA:
        //case LUA_TFUNCTION:
        //case LUA_TUSERDATA:
        //case LUA_TTHREAD:
        //case LUA_TNONE:
        default:
            return par_spack_nil( p );
        
    }
    
    // push space
    lua_pushnil( L );
    // empty table
    if( !lua_next( L, -2 ) ){
        return par_spack_empty( p );
    }
    
    // check types
    ktype = lua_type( L, -2 );
    switch( ktype ){
        case LUA_TNUMBER:
            if( par_spack_sar( p ) != 0 ){
                return -1;
            }
        break;
        case LUA_TSTRING:
            if( par_spack_smp( p ) != 0 ){
                return -1;
            }
        break;
        
        // unsupported key
        default:
            errno = ENOTSUP;
            return -1;
    }
    
    return spack_tbl( p, L, idx, ktype );
}


typedef struct {
    lua_State *L;
    lua_State *co;
    const char *errstr;
    int ref_co;
    int ref_fn;
} spack_reduce_t;


static int coreduce_lua( void *mem, size_t bytes, void *udata )
{
    spack_reduce_t *pr = (spack_reduce_t*)udata;
    int rc = 0;
    
    lstate_pushref( pr->co, pr->ref_fn );
    lua_pushlstring( pr->co, mem, bytes );
    
    // run coroutine
#if LUA_VERSION_NUM >= 502
    rc = lua_resume( pr->co, pr->L, 1 );
#else
    rc = lua_resume( pr->co, 1 );
#endif

    switch( rc ){
        case LUA_YIELD:
            pr->errstr = "could not suspend";
        break;
        
        case LUA_ERRMEM:
        case LUA_ERRERR:
        case LUA_ERRSYNTAX:
        case LUA_ERRRUN:
            pr->errstr = lua_tostring( pr->co, -1 );
        break;
    }
    lua_settop( pr->co, 0 );
    
    return -(!!rc);
}


static int pack_reduce_lua( lua_State *L )
{
    if( lua_gettop( L ) )
    {
        // memory block size
        lua_Integer blksize = luaL_optinteger( L, 3, 0 );
        par_spack_t p;
        spack_reduce_t pr = {
            .L = L,
            .co = NULL,
            .errstr = NULL,
            .ref_co = LUA_NOREF,
            .ref_fn = LUA_NOREF
        };
        par_reduce_t reducer = NULL;
        void *udata = NULL;
        int rv = 0;
        
        // check blksize
        if( blksize < 0 ){
            blksize = 0;
        }
        // check reduce function
        luaL_checktype( L, 2, LUA_TFUNCTION );
        
        // nomem error
        if( !( pr.co = lua_newthread( L ) ) ){
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            return 2;
        }
        // retain references
        pr.ref_co = lstate_ref( L );
        pr.ref_fn = lstate_ref( L );
        reducer = coreduce_lua;
        udata = (void*)&pr;
        // set one to number of argument
        lua_settop( L, 1 );
        // init par_pack_t
        if( ( rv = par_spack_init( &p, (size_t)blksize, reducer, udata ) ) == 0 )
        {
            // pack value
            if( ( rv = spack_val( &p, L, 1 ) ) == 0 )
            {
                lua_settop( L, 0 );
                if( ( rv = coreduce_lua( p.mem, p.cur, udata ) ) == 0 ){
                    lua_pushboolean( L, 1 );
                }
            }
            par_pack_dispose( &p );
        }
        // release co ref
        lstate_unref( L, pr.ref_co );
        lstate_unref( L, pr.ref_fn );
        // got error
        if( rv != 0 ){
            lua_settop( L, 0 );
            lua_pushboolean( L, 0 );
            lua_pushstring( L, strerror( errno ) );
            return 2;
        }
    }
    else {
        lua_pushboolean( L, 0 );
    }
    
    return 1;
}


static int unpack_val( lua_State *L, par_unpack_t *p );

static int unpack_tbl( lua_State *L, par_unpack_t *p, size_t len )
{
    // unpack key-value
    if( len )
    {
        while( len-- )
        {
            if( unpack_val( L, p ) != 1 ||
                unpack_val( L, p ) != 1 ){
                errno = EINVAL;
                return -1;
            }
            lua_rawset( L, -3 );
        }
    }
    else
    {
UNPACK_KV:
        switch( unpack_val( L, p ) )
        {
            case 0:
            case 2:
                break;
            case 1:
                if( unpack_val( L, p ) != 1 ){
                    return -1;
                }
                lua_rawset( L, -3 );
                goto UNPACK_KV;
            default:
                return -1;
        }
    }
    
    return 1;
}


static int unpack_val( lua_State *L, par_unpack_t *p )
{
    par_extract_t ext;
    
    // unpacking
    switch( par_unpack( p, &ext ) )
    {
        case -1:
            return -1;
        case 0:
            return 0;
        case 1:
            switch( ext.isa )
            {
                // 1 byte pack
                // nil
                case PAR_ISA_NIL:
                    lua_pushnil( L );
                    return 1;
                // boolean
                // flag = 1:true, 0:false
                case PAR_ISA_BOL:
                    lua_pushboolean( L, ext.attr );
                    return 1;
                // empty table
                case PAR_ISA_EMP:
                    lua_newtable( L );
                    return 1;
                // end-of-stream
                case PAR_ISA_EOS:
                    return 2;
                // nan
                case PAR_ISA_NAN:
                    lua_pushnumber( L, NAN );
                    return 1;
                // inf
                case PAR_ISA_INF:
                    lua_pushnumber( L, ( ext.attr ) ? -INFINITY : INFINITY );
                    return 1;
                // zero
                case PAR_ISA_I0:
                    lua_pushinteger( L, 0 );
                    return 1;
                
                // 8 byte pack
                // string
                case PAR_ISA_STR:
                    lua_pushlstring( L, ext.val.str, ext.len );
                    return 1;
                
                // 2-9 byte pack
                // number
                // endian = 1:big-endian, 0:littel-endian
                // flag = 1:sign, 0:unsign
                #define lstate_push_extint( L, ext, bit ) do { \
                    if( ext.sign ){ \
                        lua_pushinteger( L, (lua_Integer)ext.val.i##bit ); \
                    } \
                    else { \
                        lua_pushinteger( L, (lua_Integer)ext.val.u##bit ); \
                    } \
                }while(0)
                
                case PAR_ISA_I8:
                    lstate_push_extint( L, ext, 8 );
                    return 1;
                case PAR_ISA_I16:
                    lstate_push_extint( L, ext, 16 );
                    return 1;
                case PAR_ISA_I32:
                    lstate_push_extint( L, ext, 32 );
                    return 1;
                case PAR_ISA_I64:
                    lstate_push_extint( L, ext, 64 );
                    return 1;
                
                #undef lstate_push_extint
                
                case PAR_ISA_F32:
                    lua_pushnumber( L, ext.val.f32 );
                    return 1;
                case PAR_ISA_F64:
                    lua_pushnumber( L, ext.val.f64 );
                    return 1;
                
                // array
                case PAR_ISA_ARR:
                // stream array
                case PAR_ISA_SAR:
                    lua_createtable( L, (int)ext.len, 0 );
                    return unpack_tbl( L, p, ext.len );
                // map
                case PAR_ISA_MAP:
                // stream map
                case PAR_ISA_SMP:
                    lua_createtable( L, 0, (int)ext.len );
                    return unpack_tbl( L, p, ext.len );
            }
        default:
            errno = EINVAL;
            return -1;
    }
}


static int unpack_lua( lua_State *L )
{
    size_t len = 0;
    const char *mem = (const char*)luaL_checklstring( L, 1, &len );
    par_unpack_t p;
    
    par_unpack_init( &p, (void*)mem, len );
    // unpack
    if( unpack_val( L, &p ) != -1 ){
        return lua_gettop( L ) - 1;
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
        { "packReduce", pack_reduce_lua },
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

