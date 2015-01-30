local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, v;

local function genArray( len )
    local arr = {};
    for i = 1, len do
        arr[i] = i;
    end
    
    return arr;
end

-- 4 bit length array
-- 8bit type(0xE0-EF) + N sint6 value
for i = 0, 15 do
    v = genArray( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 1 + #v * 1 );
    -- check value
    bin = unpack( bin );
    ifNotEqual( inspect( bin ), inspect( v ) );
end

-- check boundary values
-- len > 15
v = genArray( 16 );
bin = ifNil( pack( v ) );
-- 8bit type(0x98) + 8 bit length value + N sint6 value
ifNotEqual( #bin, 2 + #v * 1 );
