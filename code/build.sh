#!/bin/bash

# -p supresses complaint if directory exists 
mkdir -p ../build
pushd ../build
c++ ../code/sdl_scratch.cpp -o scratch -g `sdl2-config --cflags --libs`
popd
