gcc -std=c99 -g -fPIC -I/usr/include/python2.7/ -lpython2.7 *.c -o main.o
valgrind --leak-check=full -v ./main.o
# ./main.o
echo "-- -- -- -- -- -- --"
gcc -shared -std=c99 -fPIC -I/usr/include/python2.7/ -lpython2.7 *.c -o libcpython.so
python helper.py
