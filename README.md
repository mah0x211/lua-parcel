# lua-parcel

binary serialization module.

**NOTE:**  
this module is currently under development.

---

## Installation

```sh
luarocks install parcel --from=http://mah0x211.github.io/rocks/
```

## Serialization

### bin:string, err:string = pack( val )

serializing to the corresponding parcel format.

**Parameters**

- `val`: `string`, `boolean`, `number` or `table` will be serialized to corresponding format, other data types to be serialized to `nil` data type.

**Returns**

1. `bin`: string - binary serialized data.
2. `err`: string - error string. 

**Usage**

```lua
local pack = require('parcel.pack').pack;
local tbl = {
    [325]='hello', [425]='world',
     9, 0, 63, 129, 256, 65536, 4294967296, 
    -63, -129, -256, -65536, -4294967296,
    'string',
    9.0, 0.0, 1.678, 42949.864,
    -1.678, -42949.864,
    0/0, 1/0, -1/0,
    true, false
};
local bin = assert( pack( val ) );
print( #bin ); -- 115
```


## Deserialization

### val, err = unpack( bin:string )

deserializing to the corresponding lua value.

**Parameters**

1. `bin`: string - binary serialized data.

**Returns**

1. `val`: the corresponding lua value.
2. `err`: string - error string. 

**Usage**

```lua
local inspect = require('util').inspect;
local pack = require('parcel.pack').pack;
local unpack = require('parcel.unpack').unpack;
local tbl = {
    [325]='hello', [425]='world',
     9, 0, 63, 129, 256, 65536, 4294967296, 
    -63, -129, -256, -65536, -4294967296,
    'string',
    9.0, 0.0, 1.678, 42949.864,
    -1.678, -42949.864,
    0/0, 1/0, -1/0,
    true, false
};
local bin = assert( pack( tbl ) );
local val = assert( unpack( bin ) );
print( inspect( val ) );
--[[
{
    [1] = 9,
    [2] = 0,
    [3] = 63,
    [4] = 129,
    [5] = 256,
    [6] = 65536,
    [7] = 4294967296,
    [8] = -63,
    [9] = -129,
    [10] = -256,
    [11] = -65536,
    [12] = -4294967296,
    [13] = "string",
    [14] = 9,
    [15] = 0,
    [16] = 1.678,
    [17] = 42949.864,
    [18] = -1.678,
    [19] = -42949.864,
    [20] = nan,
    [21] = inf,
    [22] = -inf,
    [23] = true,
    [24] = false,
    [325] = "hello",
    [425] = "world"
}
--]]
```

## Benchmarks

```sh
$ cd bench
$ sh ./bench.sh [/path/to/json.file]
```

## TODO

- improve memory allocation process of the pack API.
- add random access API
- add data verifier API.
- stream deserialization support.
- add data format specification document.
