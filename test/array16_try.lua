local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, v;

local function genArray( len )
    local arr = {};
    
    collectgarbage('step');
    for i = 1, len do
        arr[i] = 1;
    end
    
    return arr;
end

-- 8bit type(0x95) + 16bit length value + N sint6 value
for _, i in ipairs({ 256, 65535 }) do
    v = genArray( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 3 + #v * 1 );
    -- check value
    bin = unpack( bin );
    ifNotEqual( inspect( bin ), inspect( v ) );
end

-- check boundary values
-- len > 65535
v = genArray( 65536 );
bin = ifNil( pack( v ) );
-- 8bit type(0x96) + 32 bit length value + N sint6 value
ifNotEqual( #bin, 5 + #v * 1 );
