#!/bin/bash

LUA=luajit
BENCH_ARGS="$*"

echo "PREPROCESS"
$LUA ./preprocess.lua $BENCH_ARGS
echo "====================================================================\n"

echo "PACK"
echo "====================================================================\n"

echo "PARCEL pack"
echo "--------------------------------------------------------------------"
$LUA ./bench_parcel_pack.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "PARCEL STREAM pack"
echo "--------------------------------------------------------------------"
$LUA ./bench_parcel_packreduce.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "CMSGPACK pack"
echo "--------------------------------------------------------------------"
$LUA ./bench_msgpack_pack.lua $BENCH_ARGS
echo "====================================================================\n\n"

echo "UNPACK"
echo "====================================================================\n"

echo "PARCEL unpack"
$LUA ./bench_parcel_unpack.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "PARCEL STREAM unpack"
$LUA ./bench_parcel_unpackreduce.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "CMSGPACK unpack"
$LUA ./bench_msgpack_unpack.lua $BENCH_ARGS
echo "*********************************************************************\n"
