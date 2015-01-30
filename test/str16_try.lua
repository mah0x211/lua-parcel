local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, v;

local function genStr( len )
    local str = {};
    
    for i = 1, len do
        str[i] = 'x';
    end
    
    return table.concat( str );
end

-- 8 bit length string
-- 8bit type(0x8D) + 16 bit length value + string value
for _, i in ipairs({ 256, 65535 }) do
    v = genStr( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 3 + #v );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- str length < 256
v = genStr( 255 );
bin = ifNil( pack( v ) );
ifNotEqual( #bin, 2 + #v );
-- str length > 65535
v = genStr( 65536 );
bin = ifNil( pack( v ) );
ifNotEqual( #bin, 5 + #v );
