local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, err;

-- 0xA8
bin = ifNil( pack( nil ) );
-- check size
ifNotEqual( #bin, 1 );
-- check value
bin, err = unpack( bin );
ifNotNil( err );
ifNotEqual( bin, nil );
