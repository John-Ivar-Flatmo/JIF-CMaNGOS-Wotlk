#!/bin/bash
##compiles from scratch after deleating exiting build

##todo

scriptDir="${PWD}" ##find the real script dir

rm -r ./built
rm -r ./build

##threads=$((`nproc`+1)) ##available cores +1 to ensure fast compile time
##threads=$(((`nproc`)/2)) ##half of available threads +1 for reduced ram usage and not cook system
threads=$(((`nproc`)/4)) ##forth of available threads +1 for reduced ram usage and not cook system when builing multiple at once
##threads=2 ##more than 2 threads might interfere with other uses since we only have 4 and run servers
if [ "$threads" -eq "0" ]; then
    threads=1
fi
echo "threads: $threads"

##fix for recent commit breaking build, needing higher versions of compilers
export CC=/usr/bin/gcc-13
export CXX=/usr/bin/g++-13

cmakeFlags=" -DCMAKE_INSTALL_PREFIX=$scriptDir/built -DBUILD_EXTRACTORS=OFF -DPCH=0 -DDEBUG=1 -DBUILD_PLAYERBOT=ON -DBUILD_AHBOT=ON"
mkdir "$scriptDir/build"
cd "$scriptDir/build"
cmake "$scriptDir/" $cmakeFlags
make -j$threads
make install ##not acutaly installing just copying files to  DCMAKE_INSTALL_PREFIX



##alternate implementation, using tools built in clean command, not as thurogh but safer if building in the wrong diretory
#threads=1
#cmakeFlags=" -DCMAKE_INSTALL_PREFIX=./built -DBUILD_EXTRACTORS=OFF -DPCH=0 -DDEBUG=1 -DBUILD_PLAYERBOT=ON"
#mkdir build
#cmake --build "./build" --target clean $cmakeFlags
#cmake --build "./build" $cmakeFlags
#cd build
#make clean
#make -j$threads
#make install ##not acutaly installing just copying files to  DCMAKE_INSTALL_PREFIX
