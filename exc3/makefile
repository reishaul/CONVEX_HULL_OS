CXX = g++
CXXFLAGS = -Wall -g

all: main

main: main.o ../exc2/graham_deque.o
	$(CXX) $(CXXFLAGS) -o main main.o ../exc2/graham_deque.o

main.o: main.cpp ../exc2/graham_common.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f main main.o
	rm -f ../exc2/graham_deque.o 
