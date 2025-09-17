#include "proactor.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#define PORT 8081
#define BUFFER_SIZE 1024
//  function 
void* handleClient(int clientFD) {

    pthread_t tid=pthread_self();

    std::cout <<"thread "<<tid<< " handle client (fd=" << clientFD << ")" << std::endl;

    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(clientFD, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << "[Thread] Client disconnected (fd=" << clientFD << ")" << std::endl;
            close(clientFD);
            return nullptr;
        }
        buffer[bytes] = '\0';
        std::cout << "[Thread] Received from fd=" << clientFD << ": " << buffer;

        std::string response = "Echo: " + std::string(buffer);
        send(clientFD, response.c_str(), response.size(), 0);
    }
    return nullptr;
}

int main() {
    int serverFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFD < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(serverFD, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(serverFD, 10) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Proactor server listening on port " << PORT << "..." << std::endl;

    // הפעלת ה־Proactor עם הפונקציה שלנו
    pthread_t tid = startProactor(serverFD, handleClient);

    // מחכים (בשרת אמיתי כאן היינו רוצים לנהל את החיים של השרת)
    pthread_join(tid, nullptr);

    close(serverFD);
    return 0;
}
