CXX = g++
CXXFLAGS = -Wall -g

# Source and object files using files from exc2, exc3, and exc5
SRC = server10.cpp ../exc3/actions.cpp ../exc2/graham_deque.cpp ../exc5/proactor.cpp
OBJ = $(SRC:.cpp=.o)

# Target to build the server
all: server10

# Compile each source file into an object file
server10: $(OBJ)
	$(CXX) $(CXXFLAGS) -o server10 $(OBJ)

# Clean up object files and the server executable
clean:
	rm -f $(OBJ) server10
