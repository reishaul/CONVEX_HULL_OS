#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <functional>
#include <sys/select.h>
#include <unistd.h>
#include <map>

typedef void *(* reactorFunc) (int fd);

struct Reactor {
    fd_set master_set; // master file descriptor list
    int max_fd; // maximum file descriptor number
    std::map<int, reactorFunc> handlers; // map of fd to handler functions
    bool running; // flag to control the reactor loop
};

// starts new reactor and returns pointer to it
void * startReactor ();

// adds fd to Reactor (for reading) ; returns 0 on success.
int addFdToReactor(void * reactor, int fd, reactorFunc func);

// removes fd from reactor
int removeFdFromReactor(void * reactor, int fd);

// stops reactor
int stopReactor(void * reactor);

#endif // REACTOR_HPP