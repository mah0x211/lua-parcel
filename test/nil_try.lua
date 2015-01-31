local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin, err;

-- 0xA8
bin = ifNil( pack( nil ) );
-- check size
ifNotEqual( #bin, 1 );
-- check value
bin, err = unpack( bin );
ifNotNil( err );
ifNotEqual( bin, nil );
