local pack = require('parcel.pack');
local unpack = require('parcel.unpack');
local bin, v, size;

local function genMap( len )
    local map = {};
    local ksize = 0;
    local vsize = 0;
    local k;
    
    for i = 1, len do
        k = tostring(i);
        map[k] = i;
        
        -- k: str5 + str
        if #k < 32 then
            ksize = ksize + #k + 1;
        -- k: str8 + str
        elseif #k < 256 then
            ksize = ksize + #k + 2;
        -- k: str16 + str
        elseif #k < 65536 then
            ksize = ksize + #k + 3;
        -- k: str32 + str
        elseif #k < 4294967296 then
            ksize = ksize + #k + 5;
        -- k: str64 + str
        else
            ksize = ksize + #k + 9;
        end
        
        -- i: sint6
        if i < 64 then
            vsize = vsize + 1;
        -- i: uint8 + ival
        elseif i < 256 then
            vsize = vsize + 2;
        -- i: uint16 + ival
        elseif i < 65536 then
            vsize = vsize + 3;
        -- i: uint32 + ival
        elseif i < 4294967296 then
            vsize = vsize + 5;
        -- i: uint64 + ival
        else
            vsize = vsize + 9;
        end
    end
    
    return map, ksize + vsize;
end


-- 4 bit length map
-- 8bit type(0xF0-FF) + serialized kv-pair size
for i = 0, 15 do
    v, size = genMap( i );
    bin = ifNil( pack( v ) );
    -- check size
    ifNotEqual( #bin, 1 + size );
    -- check value
    bin = unpack( bin );
    ifNotEqual( inspect( bin ), inspect( v ) );
end

-- check boundary values
-- len > 15
v, size = genMap( 16 );
bin = ifNil( pack( v ) );
-- 8bit type(0x98) + 8bit length value + serialized kv-pair size
ifNotEqual( #bin, 2 + size );
