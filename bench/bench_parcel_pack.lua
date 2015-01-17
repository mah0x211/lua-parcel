local DATA;
do
    local fn = assert( io.open('./allthethings.json') );
    DATA = fn:read('*a');
    fn:close();
    print( 'DATA LENGTH: ' .. #DATA );
    DATA = require('cjson.safe').decode( DATA );
end

local sleep = require('process').sleep;
local pack = require('parcel').pack;
local clock = os.clock;
local cycle = _G.arg[1] or 1;
local sec, json;

print( 'cycle:', cycle );
collectgarbage('collect');
sleep(1);
sec = clock();
for i = 1, cycle do
    json = pack( DATA );
end
sec = clock() - sec;
print( sec );
