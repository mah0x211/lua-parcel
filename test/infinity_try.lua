local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin;

for _, v in ipairs({
    -1/0,   -- 0xA4
    1/0     -- 0xA5
}) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 1 );
    -- check value
    ifNotEqual( unpack( bin ), v );
end
