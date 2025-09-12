#include "reactor.hpp"
#include <iostream>
#include <thread>

/**
 * @brief The main loop of the reactor, which waits for events and dispatches them to the appropriate handlers.
 * @param arg Pointer to the Reactor instance.
 * @return nullptr
 */
void* reactorLoop(void* arg) {

    Reactor* reactor = (Reactor*)arg;//cast the void pointer to Reactor pointer
    fd_set readfds; // set of file descriptors for reading
    struct timeval tv;// timeout for select

    // main loop of the reactor
    while (reactor->running) {
        readfds = reactor->master_set;// copy the master set
        tv.tv_sec = 1;  // timeout so we can check running
        tv.tv_usec = 0;

        int activity = select(reactor->max_fd + 1, &readfds, NULL, NULL, &tv);// wait for an activity on one of the fds

        if (activity < 0) {// error handling
            perror("select");
            continue;
        }

        // check which fds are ready and call their handlers
        for (auto const& kv : reactor->handlers) {
            int fd = kv.first;// get the fd. kv is a pair (fd, func)
            reactorFunc func = kv.second;

            if (FD_ISSET(fd, &readfds)) {// if the fd is ready
                func(fd); // call the user-defined function
            }
        }
    }
    return NULL;// return nullptr when done
}

/**
 * @brief Starts a new reactor instance and runs it in a separate thread
 * @return Pointer to the newly created Reactor instance
 */
void* startReactor() {
    Reactor* reactor = new Reactor();// allocate a new Reactor instance
    reactor->running = true;// set running to true
    reactor->max_fd = 0;// initialize max_fd
    FD_ZERO(&reactor->master_set);// initialize the master set

    // start in a separate thread
    std::thread([reactor]() {reactorLoop(reactor);}).detach();//.detach to run independently

    return reactor;//return the pointer to the reactor
}

/*
* @brief Adds a file descriptor to the reactor for monitoring
* @param r Pointer to the Reactor instance
* @param fd File descriptor to be added
* @param func Handler function to be called when the fd is ready
* @return 0 on success
*/
int addFdToReactor(void* r, int fd, reactorFunc func) {
    Reactor* reactor = (Reactor*)r;//cast the void pointer to Reactor pointer
    FD_SET(fd, &reactor->master_set);// add fd to the master set
    reactor->handlers[fd] = func;// map the fd to its handler function
    if (fd > reactor->max_fd) reactor->max_fd = fd;// update max_fd if necessary
    return 0;
}

/*
 * @brief Removes a file descriptor from the reactor
 * @param r Pointer to the Reactor instance
 * @param fd File descriptor to be removed
 * @return 0 on success
 */
int removeFdFromReactor(void* r, int fd) {
    Reactor* reactor = (Reactor*)r;//cast the void pointer to Reactor pointer
    FD_CLR(fd, &reactor->master_set);// remove fd from the master set
    reactor->handlers.erase(fd);// remove the handler mapping
    return 0;
}

/*
 * @brief Stops the reactor loop
 * @param r Pointer to the Reactor instance
 * @return 0 on success
 */
int stopReactor(void* r) {
    Reactor* reactor = (Reactor*)r;//cast the void pointer to Reactor pointer
    reactor->running = false;// set running to false to stop the loop
    return 0;
}
