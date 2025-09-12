
# Compile with g++ and link with pthreads
all: example

# Build the example executable
example: reactor.o example.o
	g++ -pthread reactor.o example.o -o example

#/Compile source files into object files
reactor.o: reactor.cpp reactor.hpp
	g++ -c reactor.cpp

#Compile source files into object files
example.o: example.cpp reactor.hpp
	g++ -c example.cpp

# Clean up object files and the executable
clean:
	rm -f *.o example
