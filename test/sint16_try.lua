local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 16 bit signed integer
-- 8bit type(0x85) + 16 bit value = 3 byte
for i = -32768, -129 do
    bin = ifNil( pack( i ) );
    -- check size
    ifNotEqual( #bin, 3 );
    ifNotEqual( unpack( bin ), i );
end

-- check boundary values
-- num < -32768
bin = ifNil( pack( -32769 ) );
ifNotEqual( #bin, 5 );
-- num > -129
bin = ifNil( pack( -128 ) );
ifNotEqual( #bin, 2 );
