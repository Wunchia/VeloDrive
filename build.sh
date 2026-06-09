if [ ! -d build ]; then
  mkdir build
fi
cd build
cmake .. -DBUILD_TESTS=OFF.
make
cd ..
