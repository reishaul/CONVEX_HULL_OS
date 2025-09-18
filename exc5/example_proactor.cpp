#include "proactor.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#define PORT 8081
#define BUFFER_SIZE 1024
/*
 * Function to handle each client connection and echo received messages
 * @param clientFD The file descriptor of the client socket
 */
void* handleClient(int clientFD) {

    pthread_t tid=pthread_self();// get current thread id

    std::cout <<"thread "<<tid<< " handle client (fd=" << clientFD << ")" << std::endl;

    char buffer[BUFFER_SIZE];// buffer for reading data

    while (true) {// loop to handle client messages
        memset(buffer, 0, sizeof(buffer));// clear the buffer
        ssize_t bytes = recv(clientFD, buffer, sizeof(buffer) - 1, 0);// receive data from client

        if (bytes <= 0) {// if recv failed or connection closed
            std::cout << "Thread Client disconnected (fd=" << clientFD << ")" << std::endl;
            close(clientFD);// close client socket
            return nullptr;// exit thread
        }
        buffer[bytes] = '\0';// null-terminate the received string
        std::cout << "Thread Received from fd=" << clientFD << ": " << buffer;

        std::string response = "Echo: " + std::string(buffer);// prepare echo response
        send(clientFD, response.c_str(), response.size(), 0);// send response back to client
    }
    return nullptr;
}


// Simple main function to set up server and start proactor
int main() {
    int serverFD = socket(AF_INET, SOCK_STREAM, 0);// create server socket

    if (serverFD < 0) {// if socket creation failed
        perror("socket");
        return 1;
    }

    
    sockaddr_in addr{};// server address structure
    addr.sin_family = AF_INET;// IPv4
    addr.sin_addr.s_addr = INADDR_ANY;// bind to any address
    addr.sin_port = htons(PORT);// set port

    if (bind(serverFD, (sockaddr*)&addr, sizeof(addr)) < 0) {// if bind failed
        perror("bind");
        return 1;
    }

    if (listen(serverFD, 10) < 0) {// if listen failed
        perror("listen");
        return 1;
    }

    std::cout << "Proactor server listening on port " << PORT << "..." << std::endl;

    // start proactor with server socket and client handler function
    pthread_t tid = startProactor(serverFD, handleClient);

    // wait for proactor thread to finish
    pthread_join(tid, nullptr);

    close(serverFD);// close server socket
    return 0;
}
