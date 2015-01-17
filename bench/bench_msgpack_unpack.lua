local DATA;
do
    local fn = assert( io.open('./allthethings.json') );
    DATA = fn:read('*a');
    fn:close();
    DATA = require('cmsgpack').pack( require('cjson.safe').decode( DATA ) );
    print( 'MSGPACK PACKED DATA LENGTH: ' .. #DATA );
end

local sleep = require('process').sleep;
local unpack = require('cmsgpack').unpack;
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
