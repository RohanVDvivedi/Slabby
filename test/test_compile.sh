cd ..
make all
cd test
gcc ./test.c -o test.out -I${SLOPPY_PATH}/inc -L${SLOPPY_PATH}/bin -lsloppy -lpthread
./test.out