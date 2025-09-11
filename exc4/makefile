CXX = g++
CXXFLAGS = -Wall -g


SRC = server.cpp ../exc3/actions.cpp ../exc2/graham_deque.cpp
OBJ = $(SRC:.cpp=.o)

all: server

server: $(OBJ)
	$(CXX) $(CXXFLAGS) -o server $(OBJ) 


clean:
	rm -f $(OBJ) server

