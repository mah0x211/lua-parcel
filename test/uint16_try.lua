local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 16 bit unsigned integer
-- 8bit type(0x81) + 16 bit value = 3 byte
for i = 0xFF + 1, 0xFFFF do
    bin = ifNil( pack( i ) );
    -- check size
    ifNotEqual( #bin, 3 );
    ifNotEqual( unpack( bin ), i );
end

-- check boundary values
-- num < 256
bin = ifNil( pack( 0xFF ) );
ifNotEqual( #bin, 2 );
-- num > 65535
bin = ifNil( pack( 0xFFFF + 1 ) );
ifNotEqual( #bin, 5 );
