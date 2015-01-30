local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 16 bit unsigned integer
-- 8bit type(0x81) + 16 bit value = 3 byte
for i = 256, 65535 do
    bin = ifNil( pack( i ) );
    -- check size
    ifNotEqual( #bin, 3 );
    ifNotEqual( unpack( bin ), i );
end

-- check boundary values
-- num < 256
bin = ifNil( pack( 255 ) );
ifNotEqual( #bin, 2 );
-- num > 65535
bin = ifNil( pack( 65536 ) );
ifNotEqual( #bin, 5 );
