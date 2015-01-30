local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 8 bit signed integer
-- 8bit type(0x84) + 8 bit value = 2 byte
for i = -128, -64 do
    bin = ifNil( pack( i ) );
    -- check size
    ifNotEqual( #bin, 2 );
    ifNotEqual( unpack( bin ), i );
end

-- check boundary values
-- num < -128
bin = ifNil( pack( -129 ) );
ifNotEqual( #bin, 3 );
-- num > -64
bin = ifNil( pack( -63 ) );
ifNotEqual( #bin, 1 );
