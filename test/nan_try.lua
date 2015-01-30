local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

-- 0xA0
bin = ifNil( pack( 0/0 ) );
-- check size
ifNotEqual( #bin, 1 );
-- check value
bin = unpack( bin );
ifEqual( bin, bin );
