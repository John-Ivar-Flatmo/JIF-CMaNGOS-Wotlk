#!/bin/bash
##may need work wrote just before sleep
##cleans build directory then recompiles ##SLOW

##NOTE FOR CRACKEN
##to stop start the servers	##they are runnig se terminals called vanilla-SOMTHING or wrath-SOMTIHNG
##worldRun.sh /autorestart for world/
##world:	start(./mangosd)	stop(.server exit) ##ctrl+c might work too not sure, probly just wastes the last 5-15 min of progress, tho ive never stopped it like that that i remember
##login:	start(./realmd)		stop(ctrl+c)

##todo
##replace threads+1 to isntead check for 0 and only add 1 if its 0

scriptDir="${PWD}" ##find the real script dir

rm -r ./built
rm -r ./build

##threads=$((`nproc`+1)) ##available cores +1 to ensure fast compile time
##threads=$(((`nproc`)/4)) ##half of available threads +1 for reduced ram usage and not cook system
##threads=$((threads+1));	##add one thread incase rounds to 0
threads=2 ##more than 2 threads might interfere with other uses since we only have 4 and run servers
##if you want to compile faster move the code over to JIFFRD and run the fullcompile there with one of the other thread options
cmakeFlags=" -DCMAKE_INSTALL_PREFIX=$scriptDir/built -DBUILD_EXTRACTORS=OFF -DPCH=0 -DDEBUG=1 -DBUILD_PLAYERBOT=ON"
mkdir "$scriptDir/build"
cd "$scriptDir/build"
cmake $scriptDir/.. $cmakeFlags
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