local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 6 bit signed integer
-- positive: 0x00-3F
-- negative: 0x40-7F
for i = -63, 63 do
    bin = ifNil( pack( i ) );
    -- check size
    ifNotEqual( #bin, 1 );
    ifNotEqual( unpack( bin ), i );
end

-- check boundary values
-- num < -63
bin = ifNil( pack( -64 ) );
ifNotEqual( #bin, 2 );
-- num > 63
bin = ifNil( pack( 64 ) );
ifNotEqual( #bin, 2 );
