rm -r testdb
cd ../pf
make
cd ../hf
make
cd ../fe
make clean
make
./fetest-ddl
