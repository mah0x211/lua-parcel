local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin;

-- 64 bit signed integer
-- 8bit type(0x87) + 64 bit value = 9 byte
for _, v in ipairs({ -9223372036854775808, -2147483649 }) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 9 );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- num > -2147483649
bin = ifNil( pack( -2147483648 ) );
ifNotEqual( #bin, 5 );
