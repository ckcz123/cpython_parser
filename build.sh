gcc -std=c99 -g -fPIC -I/usr/include/python2.7/ -lpython2.7 *.c -o main.o
./main.o
echo "-- -- -- -- -- -- --"
gcc -shared -std=c99 -fPIC -I/usr/include/python2.7/ -lpython2.7 *.c -o libcpython.so
python main.py
