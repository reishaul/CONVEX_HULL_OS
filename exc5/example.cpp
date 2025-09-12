#include "reactor.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

// Example handler function for stdin
void* stdinHandler(int fd) {
    char buf[1024];//buffer to read input
    int n = read(fd, buf, sizeof(buf)-1);//int to hold number of bytes read
    if (n > 0) {//if read was successful
        buf[n] = '\0';
        std::cout << "Got input: " << buf;//feedback the input
    }
    return NULL;
}

// Example usage of the reactor
int main() {
    void* reactor = startReactor();//start the reactor that runs with select in a separate thread
    addFdToReactor(reactor, STDIN_FILENO, stdinHandler);//write the fd and the handler function to the reactor

    //when there is a activity on stdin, the reactor will call stdinHandler
    std::cout << "Type something (CTRL+C to exit):\n";
    while (true) { } // Keep the main thread alive
}
