#include "proactor.hpp"
#include <unistd.h>
#include <iostream>

#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*
 * Structure to hold data for each proactor instance
* Contains the server socket file descriptor and the function to handle each client
 */
struct ProactorArgs {
    int serverFD;// server socket file descriptor
    proactorFunc func;// function to handle each client
};


/*
 * Thread function to handle client connections
 * and delegate to the provided function
 * @param arg Pointer to ProactorArgs structure
 */
void* proactor_loop(void* arg) {
    ProactorArgs* args = (ProactorArgs*) arg;// cast argument to ProactorArgs pointer
    int server_fd = args->serverFD;// get server socket file descriptor
    proactorFunc func = args->func;// get client handling function

    std::cout << "Proactor Started listening for new clients..." << std::endl;

    while (true) {//loop over incoming connections
        sockaddr_in client_addr{};// client address structure
        socklen_t addr_len = sizeof(client_addr);// length of client address
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &addr_len);// accept new client connection and get client socket file descriptor
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Proactor New client connected (fd=" << client_fd << ")" << std::endl;// print new client connection message

        pthread_t tid;// thread id for the new client handler thread
        pthread_create(&tid, nullptr, (void*(*)(void*))func, (void*)(intptr_t)client_fd);// create a new thread to handle the client using the provided function
        pthread_detach(tid); // no need for pthread_join
    }
    return nullptr;
}

/**
 * Starts a new proactor and returns the proactor thread id
 * and takes the server socket file descriptor and the function to handle each client
 * @param sockfd  The server socket file descriptor
 * @param  proactorFunc  The function to handle each client
 */
pthread_t startProactor(int sockfd, proactorFunc threadFunc) {
    pthread_t tid;// thread id for the proactor thread
    ProactorArgs* args = new ProactorArgs{sockfd, threadFunc};// allocate and initialize ProactorArgs structure
    pthread_create(&tid, nullptr, proactor_loop, args);// create the proactor thread

    return tid;
}

/**
 * Stops the proactor by cancelling the thread
 * @param tid The thread id of the proactor to stop
 * @return 0 on success, error code on failure
 */
int stopProactor(pthread_t tid) {
    return pthread_cancel(tid);
}

