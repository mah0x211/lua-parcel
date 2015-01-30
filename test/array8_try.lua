local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, v;

local function genArray( len )
    local arr = {};
    for i = 1, len do
        arr[i] = 1;
    end
    
    return arr;
end

-- 8bit type(0x94) + 8bit length value + N sint6 value
for i = 16, 255 do
    v = genArray( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 2 + #v * 1 );
    -- check value
    bin = unpack( bin );
    ifNotEqual( inspect( bin ), inspect( v ) );
end

-- check boundary values
-- len > 255
v = genArray( 256 );
bin = ifNil( pack( v ) );
-- 8bit type(0x95) + 16 bit length value + N sint6 value
ifNotEqual( #bin, 3 + #v * 1 );
