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

-- 32 bit length array
-- 8bit type(0x96) + 32bit length value + N sint6 value
for _, i in ipairs({
    65536, 
    -- lua cannot be handled more than 1<<MAXBITS(26) length array
    -- 0x4000000000
}) do
    v = genArray( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 5 + #v * 1 );
    -- check value
    bin = unpack( bin );
    ifNotEqual( inspect( bin ), inspect( v ) );
end

