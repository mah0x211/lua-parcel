local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, v, klen;

local function genMap( len )
    local map = {};
    local klen = 0;
    local k;
    
    for i = 1, len do
        k = ''..i;
        map[k] = i;
        klen = klen + #k;
    end
    
    return map, klen + len;
end

-- 4 bit length map
-- 8bit type(0xF0-FF) + key(str5(8bit)+strlen) + val(sint6(8bit))
for i = 0, 15 do
    v, klen = genMap( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 1 + klen + ( i * 1 ) );
    -- check value
    bin = unpack( bin );
    ifNotEqual( inspect( bin ), inspect( v ) );
end

-- check boundary values
-- len > 15
v, klen = genMap( 16 );
bin = ifNil( pack( v ) );
-- 8bit type(0x98) + 8 bit length + key(str5(8bit)+strlen) + val(sint6(8bit))
ifNotEqual( #bin, 2 + klen + 16 );
