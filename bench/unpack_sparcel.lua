local unpack = require('parcel.unpack').unpack;
local sleep = require('process').sleep;
local clock = require('process').gettimeofday;
local MB = 1024 * 1024;
local cycle = _G.arg[2] or 1;
local DATA = _G.arg[1];
local sec, res;

local function printSize( fmt, byte )
    print( fmt:format( byte, byte / MB ) );
end

local function reduce( size, bin )
    res = res .. bin;
    collectgarbage('step');
end

print( 'CYCLE:', cycle );
print( 'TEST FILE: ' .. DATA );
do
    local fn = assert( io.open( DATA .. '.sparcel' ) );
    DATA = fn:read('*a');
    fn:close();
    printSize( 'DATA SIZE: %d B | %f MB', #DATA );
end
print( '==============================' );
print( 'collectgarbage and sleep 1 sec' );
print( '==============================' );
collectgarbage('collect');

sleep(1);

sec = clock();
for i = 1, cycle do
    res = unpack( DATA );
end
sec = clock() - sec;
print( ('COST: %f sec'):format( sec ) );
