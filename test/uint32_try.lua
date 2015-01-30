local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 32 bit signed integer
-- 8bit type(0x82) + 32 bit value = 5 byte
for _, v in ipairs({ 65536, 4294967295 }) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 5 );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- num < 65536
bin = ifNil( pack( 65535 ) );
ifNotEqual( #bin, 3 );
-- num > 4294967295
bin = ifNil( pack( 4294967296 ) );
ifNotEqual( #bin, 9 );
