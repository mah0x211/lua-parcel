local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local bin;

-- 64 bit float(double)
-- 8bit type(0xA3) + 64 bit value = 9 byte
for _, v in ipairs({ 0.1, 1.7976931348623158E+308, 2.2204460492503131E-16 }) do
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 9 );
    ifNotEqual( unpack( bin ), v );
end
