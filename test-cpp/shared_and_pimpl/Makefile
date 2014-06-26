
main.o: main.cpp
	g++ --std=c++11 -c main.cpp

s.o: s.cpp
	g++ --std=c++11 -c s.cpp

a.bin: main.o s.o
	g++ --std=c++11 main.o s.o -o a.bin

run: a.bin
	./a.bin

