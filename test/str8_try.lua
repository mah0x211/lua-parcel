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
-- 8bit type(0x8C) + 8 bit length value + string value
for i = 32, 255 do
    v = genStr( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 2 + #v );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- str length < 32
v = genStr( 31 );
bin = ifNil( pack( v ) );
ifNotEqual( #bin, 1 + #v );
-- str length > 255
v = genStr( 256 );
bin = ifNil( pack( v ) );
ifNotEqual( #bin, 3 + #v );
