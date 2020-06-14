cd ..
make all
cd test
gcc ./test.c -o test.out -I${SLOPPY_PATH}/inc -I${CUTLERY_PATH}/inc -L${SLOPPY_PATH}/bin -L${CUTLERY_PATH}/bin -lsloppy -lpthread -lcutlery
./test.out