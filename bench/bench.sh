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
$LUA ./pack_parcel.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "PARCEL STREAM pack"
echo "--------------------------------------------------------------------"
$LUA ./pack_sparcel.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "CMSGPACK pack"
echo "--------------------------------------------------------------------"
$LUA ./pack_msgpack.lua $BENCH_ARGS
echo "====================================================================\n\n"

echo "UNPACK"
echo "====================================================================\n"

echo "PARCEL unpack"
$LUA ./unpack_parcel.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "PARCEL STREAM unpack"
$LUA ./unpack_sparcel.lua $BENCH_ARGS
echo "*********************************************************************\n"

echo "CMSGPACK unpack"
$LUA ./unpack_msgpack.lua $BENCH_ARGS
echo "*********************************************************************\n"
