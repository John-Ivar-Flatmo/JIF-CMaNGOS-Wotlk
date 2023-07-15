##compiles script even if its already been compiled ##fast if already been compiled before

rm -r ./built

threads=1
cmakeFlags=" -DCMAKE_INSTALL_PREFIX=./built -DBUILD_EXTRACTORS=OFF -DPCH=0 -DDEBUG=1 -DBUILD_PLAYERBOT=ON"
mkdir build
cd build
cmake .. $cmakeFlags
make -j$threads
make install ##not acutaly installing just copying files to  DCMAKE_INSTALL_PREFIX
