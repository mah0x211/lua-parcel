local PATH = _G.arg[1];
local decode = require('cjson.safe').decode;
    
local function genParcel( json )
    local pack = require('parcel.pack').pack;
    local fh = assert( io.open( PATH .. '.parcel', 'w' ) );
    local res = pack( json );
    
    fh:write( res );
    fh:close();
end

    
local function genSParcel( json )
    local fh = assert( io.open( PATH .. '.sparcel', 'w' ) );
    local reduce = function( size, bin )
        fh:write( bin );
    end;
    local pack = require('parcel.stream.pack').new( reduce );
    
    pack( json );
    fh:close();
end


local function genMsgPack( json )
    local pack = require('cmsgpack').pack;
    local fh = assert( io.open( PATH .. '.msgpack', 'w' ) );
    local res = pack( json );
    
    fh:write( res );
    fh:close();
end

local function decodeJSON( json )
    local json = require('cjson.safe').decode( json );

end

local function generate()
    local fn = assert( io.open( PATH ) );
    local json = fn:read('*a');
    local clock = require('process').gettimeofday;
    local sec;
    
    fn:close();
    print( 'JSON.decode ' .. PATH );
    sec = clock();
    json = decode( json );
    sec = clock() - sec;
    print( ('decoded: %f sec'):format( sec ) );
    for k, fn in pairs({
        msgpack = genMsgPack,
        parcel = genParcel,
        parcelStream = genSParcel
    }) do
        collectgarbage('collect');
        sec = clock();
        fn( json );
        sec = clock() - sec;
        print( (k .. ' binary has been generated | %f sec'):format( sec ) );
    end
end

generate();
