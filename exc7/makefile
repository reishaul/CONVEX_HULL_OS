CXX = g++
CXXFLAGS = -Wall -g

# Source and object files using files from exc2 and exc3
SRC = thread_server.cpp ../exc3/actions.cpp ../exc2/graham_deque.cpp
OBJ = $(SRC:.cpp=.o)

# Target to build the server
all: thread_server

# Compile each source file into an object file
thread_server: $(OBJ)
	$(CXX) $(CXXFLAGS) -o thread_server $(OBJ)

# Clean up object files and the server executable
clean:
	rm -f $(OBJ) thread_server
