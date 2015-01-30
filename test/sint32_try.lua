local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 32 bit signed integer
-- 8bit type(0x86) + 32 bit value = 5 byte
for _, v in ipairs({ -2147483648, -32769 }) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 5 );
    ifNotEqual( unpack( bin ), v );
end

-- check boundary values
-- num < -2147483648
bin = ifNil( pack( -2147483649 ) );
ifNotEqual( #bin, 9 );
-- num > -32769
bin = ifNil( pack( -32768 ) );
ifNotEqual( #bin, 3 );
