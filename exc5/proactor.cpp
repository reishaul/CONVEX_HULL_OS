#include "proactor.hpp"
#include <unistd.h>
#include <iostream>

#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*
 * Structure to hold data for each proactor instance
 *
 */
struct ProactorArgs {
    int serverFD;
    proactorFunc func;//
};


void* proactor_loop(void* arg) {
    ProactorArgs* args = (ProactorArgs*) arg;
    int server_fd = args->serverFD;
    proactorFunc func = args->func;

    std::cout << "[Proactor] Started listening for new clients..." << std::endl;

    while (true) {//loop 
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "[Proactor] New client connected (fd=" << client_fd << ")" << std::endl;

        pthread_t tid;
        pthread_create(&tid, nullptr, (void*(*)(void*))func, (void*)(intptr_t)client_fd);
        pthread_detach(tid); // לא צריך pthread_join
    }
    return nullptr;
}

/**
 * Starts a new proactor and returns the proactor thread id
 * @param int sockfd
 * @param  proactorFunc 
 * 
 */
pthread_t startProactor(int sockfd, proactorFunc threadFunc) {
    pthread_t tid;
    ProactorArgs* args = new ProactorArgs{sockfd, threadFunc};
    pthread_create(&tid, nullptr, proactor_loop, args);

    return tid;
}

int stopProactor(pthread_t tid) {
    return pthread_cancel(tid);
}

