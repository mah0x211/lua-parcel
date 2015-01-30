local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin;

for _, v in ipairs({
    true,   -- 0xA6
    false   -- 0xA7
}) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 1 );
    -- check value
    ifNotEqual( unpack( bin ), v );
end
