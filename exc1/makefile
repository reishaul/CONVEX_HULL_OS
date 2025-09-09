CXX = g++
CXXFLAGS = -Wall -g

main: main.o graham.o
	$(CXX) $(CXXFLAGS) -o main main.o graham.o

main.o: main.cpp graham.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

graham.o: graham.cpp graham.hpp
	$(CXX) $(CXXFLAGS) -c graham.cpp

run: main
	./main

clean:
	rm -f main *.o

