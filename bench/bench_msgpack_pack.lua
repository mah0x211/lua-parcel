local pack = require('cmsgpack').pack;
local sleep = require('process').sleep;
local clock = require('process').gettimeofday;
local MB = 1024 * 1024;
local cycle = _G.arg[2] or 1;
local DATA = _G.arg[1];
local sec, res;

local function printSize( fmt, byte )
    print( fmt:format( byte, byte / MB ) );
end

print( 'CYCLE:', cycle );
print( 'TEST FILE: ' .. DATA );
do
    local fn = assert( io.open( DATA ) );
    DATA = fn:read('*a');
    fn:close();
    printSize( 'DATA SIZE: %d B | %f MB', #DATA );
    DATA = require('cjson.safe').decode( DATA );
end
print( '==============================' );
print( 'collectgarbage and sleep 1 sec' );
print( '==============================' );
collectgarbage('collect');
sleep(1);

sec = clock();
for i = 1, cycle do
    res = pack( DATA );
end
sec = clock() - sec;
printSize( 'PACKED SIZE: %d B | %f MB', #res );
print( ('COST: %f sec'):format( sec ) );
