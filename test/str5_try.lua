local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin, v, c;

-- 5 bit length string
-- 8bit type(0xC0-DF) + string value
for i = 0, 31 do
    c = i == 0 and '' or 'x';
    v = ('%#' .. i .. 's'):format( c );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 1 + #v );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- str length > 31
v = ('%#' .. 32 .. 's'):format( c );
bin = ifNil( pack( v ) );
-- 8bit type(0x8C) + 8 bit length value + string value = 34
ifNotEqual( #bin, 34 );
