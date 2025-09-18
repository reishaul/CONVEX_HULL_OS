CXX = g++
CXXFLAGS = -Wall -g -pthread -std=c++17

# Source and object files using files from exc2, exc3, and exc5
SRC = server9.cpp ../exc3/actions.cpp ../exc2/graham_deque.cpp ../exc5/proactor.cpp
OBJ = $(SRC:.cpp=.o)

# Target to build the server
all: server9

# Compile each source file into an object file
server9: $(OBJ)
	$(CXX) $(CXXFLAGS) -o server9 $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and the server executable
clean:
	rm -f $(OBJ) server9
