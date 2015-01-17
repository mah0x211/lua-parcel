#!/bin/bash

CYCLE=100
LUA=luajit

echo "bench #${CYCLE}"
echo "--------------------------------------------------------------------"
echo "cmsgpack: pack"
time $LUA ./bench_msgpack_pack.lua $CYCLE
echo "--------------------------------------------------------------------"
echo "cmsgpack: unpack"
time $LUA ./bench_msgpack_unpack.lua $CYCLE
echo "--------------------------------------------------------------------"

echo "parcel reduce pack"
time $LUA ./bench_parcel_packreduce.lua $CYCLE
echo "--------------------------------------------------------------------"
echo "parcel reduce unpack"
time $LUA ./bench_parcel_unpackreduce.lua $CYCLE
echo "--------------------------------------------------------------------"

echo "parcel pack"
time $LUA ./bench_parcel_pack.lua $CYCLE
echo "parcel unpack"
time $LUA ./bench_parcel_unpack.lua $CYCLE
echo "--------------------------------------------------------------------"
