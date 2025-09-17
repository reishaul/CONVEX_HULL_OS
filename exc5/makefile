# Compile with g++ and link with pthreads
CXX = g++
CXXFLAGS = -Wall -g -pthread

all: example example_proactor

# Build the reactor example executable
example: reactor.o example.o
	$(CXX) $(CXXFLAGS) reactor.o example.o -o example

# Build the proactor example executable
example_proactor: proactor.o example_proactor.o
	$(CXX) $(CXXFLAGS) proactor.o example_proactor.o -o example_proactor

# Compile source files into object files
reactor.o: reactor.cpp reactor.hpp
	$(CXX) $(CXXFLAGS) -c reactor.cpp

example.o: example.cpp reactor.hpp
	$(CXX) $(CXXFLAGS) -c example.cpp

proactor.o: proactor.cpp proactor.hpp
	$(CXX) $(CXXFLAGS) -c proactor.cpp

example_proactor.o: example_proactor.cpp proactor.hpp
	$(CXX) $(CXXFLAGS) -c example_proactor.cpp

# Clean up object files and the executables
clean:
	rm -f *.o example example_proactor
