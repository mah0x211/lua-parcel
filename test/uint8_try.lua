local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 8 bit unsigned integer
-- 8bit type(0x80) + 8 bit value = 2 byte
for i = 64, 255 do
    bin = ifNil( pack( i ) );
    -- check size
    ifNotEqual( #bin, 2 );
    ifNotEqual( unpack( bin ), i );
end

-- check boundary values
-- num < 64
bin = ifNil( pack( 63 ) );
ifNotEqual( #bin, 1 );
-- num > 255
bin = ifNil( pack( 256 ) );
ifNotEqual( #bin, 3 );
