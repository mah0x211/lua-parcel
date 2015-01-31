local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin;

-- 32 bit signed integer
-- 8bit type(0x82) + 32 bit value = 5 byte
for _, v in ipairs({ 0xFFFF + 1, 0xFFFFFFFF }) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 5 );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- num < 65536
bin = ifNil( pack( 0xFFFF ) );
ifNotEqual( #bin, 3 );
-- num > 4294967295
bin = ifNil( pack( 0xFFFFFFFF + 1 ) );
ifNotEqual( #bin, 9 );
