CXX = g++
CXXFLAGS = -Wall -g -pg

all: main_deque main_list

main_deque: main.o graham_deque.o
	$(CXX) $(CXXFLAGS) -o main_deque main.o graham_deque.o

main_list: main.o graham_list.o
	$(CXX) $(CXXFLAGS) -o main_list main.o graham_list.o

main.o: main.cpp graham_common.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

graham_deque.o: graham_deque.cpp graham_common.hpp
	$(CXX) $(CXXFLAGS) -c graham_deque.cpp

graham_list.o: graham_list.cpp graham_common.hpp
	$(CXX) $(CXXFLAGS) -c graham_list.cpp

run_deque: main_deque
	./main_deque

run_list: main_list
	./main_list

clean:
	rm -f graham_deque graham_list *.o gmon.out profile_*.txt
