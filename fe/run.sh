rm -r testdb
cd ../bf
make
cd ../pf
make
cd ../hf
make
cd ../am
make
cd ../fe
make clean
make
./fetest-ddl
