CXX = g++
CXXFLAGS = -Wall -g

# Source files with paths
SRC = ../exc5/reactor.cpp reactor_server.cpp ../exc3/actions.cpp ../exc2/graham_deque.cpp

# Object files corresponding to source files
OBJ = reactor.o reactor_server.o actions.o graham_deque.o

# Target to build the server
all: reactor_server

# Compile each object file
reactor.o: ../exc5/reactor.cpp
	$(CXX) $(CXXFLAGS) -c ../exc5/reactor.cpp -o reactor.o

reactor_server.o: reactor_server.cpp
	$(CXX) $(CXXFLAGS) -c reactor_server.cpp -o reactor_server.o

actions.o: ../exc3/actions.cpp
	$(CXX) $(CXXFLAGS) -c ../exc3/actions.cpp -o actions.o

graham_deque.o: ../exc2/graham_deque.cpp
	$(CXX) $(CXXFLAGS) -c ../exc2/graham_deque.cpp -o graham_deque.o

# Link object files into the executable
reactor_server: $(OBJ)
	$(CXX) $(CXXFLAGS) reactor.o reactor_server.o actions.o graham_deque.o -o reactor_server

# Clean up
clean:
	rm -f $(OBJ) reactor_server
