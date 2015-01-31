local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 64 bit signed integer
-- 8bit type(0x83) + 64 bit value = 9 byte
for _, v in ipairs({ 0xFFFFFFFF + 1, 0xFFFFFFFFFFFFFFFF }) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 9 );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- num < 4294967296
bin = ifNil( pack( 0xFFFFFFFF ) );
ifNotEqual( #bin, 5 );
