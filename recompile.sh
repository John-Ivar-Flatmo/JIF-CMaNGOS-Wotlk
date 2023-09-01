#!/bin/bash
##compiles even if its already been compiled ##fast if already been compiled before

##todo

scriptDir="${PWD}" ##find the real script dir

rm -r ./built

threads=1  ##single threaded to see code errors cleaner
echo "threads: $threads"

##fix for recent commit breaking build, needing higher versions of compilers
export CC=/usr/bin/gcc-13
export CXX=/usr/bin/g++-13

cmakeFlags=" -DCMAKE_INSTALL_PREFIX=./built -DBUILD_EXTRACTORS=OFF -DPCH=1 -DDEBUG=1 -DBUILD_PLAYERBOT=ON -DBUILD_AHBOT=ON"
mkdir "$scriptDir/build"
cd "$scriptDir/build"
cmake "$scriptDir/" $cmakeFlags
make -j$threads
make install ##not acutaly installing just copying files to  DCMAKE_INSTALL_PREFIX
