local DATA;
do
    local fn = assert( io.open('./allthethings.json') );
    local tbl = {};
    
    DATA = fn:read('*a');
    fn:close();
    DATA = require('cjson.safe').decode( DATA );
    require('parcel').pack( DATA, nil, function( bin )
        tbl[#tbl+1] = bin;
    end);
    DATA = table.concat( tbl );
    print( 'PARCEL REDUCE PACKED DATA LENGTH: ' .. #DATA );
end

local sleep = require('process').sleep;
local unpack = require('parcel').unpack;
local clock = os.clock;
local cycle = _G.arg[1] or 1;
local sec, json;

print( 'cycle:', cycle );
collectgarbage('collect');
sleep(1);
sec = clock();
for i = 1, cycle do
    json = unpack( DATA );
end
sec = clock() - sec;
print( sec );
