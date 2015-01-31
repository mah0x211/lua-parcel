local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin, v;

local function genStr( len )
    local str = {};
    
    for i = 1, len do
        str[i] = 'x';
    end
    
    return table.concat( str );
end

-- 8 bit length string
-- 8bit type(0x8E) + 32 bit length value + string value
for _, i in ipairs({ 
    65536, 
    -- lua cannot be handled more than 4GB
    -- 4294967295
}) do
    v = genStr( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 5 + #v );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- str length < 65536
v = genStr( 65535 );
bin = ifNil( pack( v ) );
ifNotEqual( #bin, 3 + #v );
-- lua cannot be handled more than 4GB
--[[
-- str length > 4294967295
v = genStr( 4294967296 );
bin = ifNil( pack( v ) );
ifNotEqual( #bin, 9 + #v );
--]]
