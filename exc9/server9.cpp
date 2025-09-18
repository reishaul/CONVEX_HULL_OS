#include "../exc5/proactor.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <pthread.h>
#include <sstream>  
#include <mutex>
#include "../exc3/actions.hpp" // Include the actions header file


using namespace std;
mutex cout_mutex;// Mutex for synchronizing console output

/*
@brief Sends a message to the specified client socket(convenience function)
@param client_fd The file descriptor of the client socket
@param msg The message to send
*/
void send_to_client(int client_fd, const string &msg) {

    size_t total_sent = 0;// total bytes sent
    const char *data = msg.c_str();// get C-style string from std::string
    size_t len = msg.size();// length of the message
    while (total_sent < len) {// Ensure all data is sent
        ssize_t sent = send(client_fd, data + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            // error or connection closed
            break;
        }
        total_sent += sent;// update total bytes sent
    }
}

void *client_thread(int client_fd) {
    pthread_t tid = pthread_self();


    cout << "thread: " << pthread_self() << " handling client (fd=" << client_fd << ")" << endl;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];// buffer for reading data

    send_to_client(client_fd, "Welcome to the Convex Hull Server! (quit to end)\n");

    while (true) {// loop to handle client messages
        memset(buffer, 0, sizeof(buffer));// clear the buffer
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);// receive data from client (ssize_t is a signed type)

        if (bytes <= 0) {// if recv failed or connection closed
            if (bytes == 0) std::cout << "Thread " << tid << "client closed (fd=" << client_fd << ")\n";
            else perror("recv");
            
            close(client_fd);// close client socket
            return nullptr;// exit thread
        }

        string line(buffer);

        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();// trim newline/cr (lefties in the line)
        if(line.empty()) continue;// ignore empty lines

        cout<< "thread: " << pthread_self() << " received : " << buffer;

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "Newgraph") {
            int n;
            if (ss >> n && n > 0) {
                // lock the graph for the entire newgraph init -> consistent state
                {
                    lock_guard<std::mutex> lock(cout_mutex);
                    initGraph();
                }
                // now read n points from this client (blocking reads)
                send_to_client(client_fd, ("send " + to_string(n) + " points\n").c_str());
                int remaining = n;
                while (remaining > 0) {
                    memset(buffer, 0, sizeof(buffer));
                    ssize_t b = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    if (b <= 0) { close(client_fd); return nullptr; }
                    std::string pline(buffer);
                    while (!pline.empty() && (pline.back() == '\n' || pline.back() == '\r')) pline.pop_back();
                    std::stringstream pss(pline);
                    double x, y;
                    if (pss >> x >> y) {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        newPoint(x, y);
                        remaining--;
                        send_to_client(client_fd, "Point received\n");
                    } else {
                        send_to_client(client_fd, "Error: Invalid point format Use: x y\n");
                    }
                }
                send_to_client(client_fd, "Graph created\n");
            } else {
                send_to_client(client_fd, "Invalid number of points\n");
            }
        }
        else if (command == "Newpoint") {
            std::string coords;
            if (ss >> coords) {
                for (char &c : coords) if (c == ',') c = ' ';
                std::stringstream cs(coords);
                double x, y;
                if (cs >> x >> y) {
                    lock_guard<std::mutex> lock(cout_mutex);
                    newPoint(x, y);
                    send_to_client(client_fd, ("Point (" +to_string(x) + ", " + to_string(y) + ") added\n"));
                } else {
                    send_to_client(client_fd, "Error: Invalid point format Use: Newpoint x,y\n");
                }
            } else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Newpoint x,y\n");
            }
        }
        else if (command == "Removepoint") {
            string coords;
            if (ss >> coords) {
                for (char &c : coords) if (c == ',') c = ' ';
                std::stringstream cs(coords);
                double x, y;
                if (cs >> x >> y) {
                    lock_guard<std::mutex> lock(cout_mutex);
                    removePoint(x, y);
                    send_to_client(client_fd, ("Point (" + to_string(x) + ", " + to_string(y) + ") removed\n"));
                } else {
                    send_to_client(client_fd, "Error: Invalid point format Use: Removepoint x,y\n");
                }
            } else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Removepoint x,y\n");
            }
        }
        else if (command == "CH") {
            // protect graph while computing hull
            lock_guard<std::mutex> lock(cout_mutex);
            vector<Point> hull = grahamHull(points);
            double area = polygonArea(hull);
            stringstream resp;
            resp << "Convex Hull:\n";
            for (auto &p : hull) resp << "(" << p.x << ", " << p.y << ")\n";
            resp.setf(ios::fixed); resp.precision(3);
            resp << "Area: " << area << "\n";
            string s = resp.str();
            send_to_client(client_fd, s.c_str());
        }
        else if (command == "quit" || command == "exit") {
            send_to_client(client_fd, "bye!\n");
            close(client_fd);
            return nullptr;
        }
        else {
            send_to_client(client_fd, "Error: command isn't valid\n");
        }
    } // while
    return nullptr;
}
        

int main() {

    // create listening socket
    const int PORT = 9034;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    // set up server address structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    int a = 1; //integer to set the option
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(a));

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 18) < 0) {//18 is the maximum number of pending connections
        perror("listen");
        return 1;
    }

    cout << "Server listening on port 9034..." << std::endl;

    // using the proactor to handle clients from the library
    pthread_t proactorTid=startProactor(server_fd, client_thread);

    // wait for the proactor thread to finish
    pthread_join(proactorTid, nullptr);

    close(server_fd);

    return 0;
}