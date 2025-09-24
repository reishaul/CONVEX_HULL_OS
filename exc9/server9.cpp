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
mutex graph_mutex;// Mutex for synchronizing console output

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

/*
* @brief Thread function to handle client commands and manage client state
* @param client_fd The file descriptor of the client socket
* @return nullptr
*/
void *client_thread(int client_fd) {

    pthread_t tid = pthread_self();// get thread id

    cout << "thread: " << pthread_self() << " handling client (fd=" << client_fd << ")" << endl;
    const int BUFFER_SIZE = 1024;// buffer size
    char buffer[BUFFER_SIZE];// buffer for reading data

    send_to_client(client_fd, "Welcome to the Convex Hull Server! (quit to end)\n");

    bool awaiting_points = false;//waiting for points after Newgraph
    int points_needed = 0;// number of points still needed to complete the graph

    while (true) {// loop to handle client messages
        memset(buffer, 0, sizeof(buffer));// clear the buffer
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);// receive data from client (ssize_t is a signed type)

        if (bytes <= 0) {// if recv failed or connection closed
            if (bytes == 0) cout << "Thread " << tid << "client closed (fd=" << client_fd << ")\n";
            else perror("recv");
            
            close(client_fd);// close client socket
            return nullptr;// exit thread
        }

        string line(buffer);// convert buffer to string

        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();// trim newline/cr (lefties in the line)
        if(line.empty()) continue;//ignore empty lines

        cout<< "thread: " << pthread_self() << " received : " << buffer;

        if (awaiting_points) {// If waiting for points after Newgraph (to lock the graph when adding points)
            double x, y;
            stringstream ss(line);
            if (ss >> x >> y) {
                // Lock mutex when adding points to shared graph
                lock_guard<mutex> lock(graph_mutex);
                newPoint(x, y);
                points_needed--;// Decrement number of points still needed
                
                if (points_needed == 0) {// If all points received
                    awaiting_points = false;
                    send_to_client(client_fd, "Graph created successfully\n");
                    
                } 
                else {
                    send_to_client(client_fd, "Point received. " + to_string(points_needed) + " more points needed.\n");//to string is for int to string conversion
                }
            } else {
                send_to_client(client_fd, "Error: Invalid point format. Use: x y\n");
            }
            continue;
        }

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "Newgraph") {
            int n;//store number of points
            if (ss >> n && n > 0) {
                // Lock the graph for the ENTIRE newgraph process - no other client can access
                graph_mutex.lock();
                
                initGraph();// Initialize empty graph
                send_to_client(client_fd, ("send " + to_string(n) + " points\n").c_str());
                
                int remaining = n;// points remaining to read
                while (remaining > 0) {
                    memset(buffer, 0, sizeof(buffer));
                    ssize_t b = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    if (b <= 0) { 
                        graph_mutex.unlock(); // Unlock on error!
                        close(client_fd); 
                        return nullptr; 
                    }
                    string pline(buffer);// convert buffer to string
                    while (!pline.empty() && (pline.back() == '\n' || pline.back() == '\r')) pline.pop_back();
                    stringstream pss(pline);// stringstream for parsing point call it pss
                    double x, y;
                    if (pss >> x >> y) {// if successfully parsed two doubles
                        newPoint(x, y);  // No separate lock needed - we already have the mutex
                        remaining--;// one less point to read
                        send_to_client(client_fd, "Point received\n");
                    }
                    else {
                        send_to_client(client_fd, "Error: Invalid point format Use: x y\n");
                    }
                }
                
                // Unlock only after ALL points are added
                graph_mutex.unlock();
                send_to_client(client_fd, "Graph created\n");
            } else {
                send_to_client(client_fd, "Invalid number of points\n");
            }
        }
        else if (command == "Newpoint") {
            string coords;
            if (ss >> coords) {
                for (char &c : coords) if (c == ',') c = ' ';
                stringstream cs(coords);// stringstream for parsing point call it cs
                double x, y;
                if (cs >> x >> y) {
                    lock_guard<mutex> lock(graph_mutex);// lock the mutex
                    newPoint(x, y);
                    send_to_client(client_fd, ("Point (" +to_string(x) + ", " + to_string(y) + ") added\n"));
                }//here the mutex is unlocked
                else {
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
                stringstream cs(coords);// stringstream for parsing point call it cs
                double x, y;
                if (cs >> x >> y) {
                    lock_guard<mutex> lock(graph_mutex);// lock the mutex 
                    removePoint(x, y);
                    send_to_client(client_fd, ("Point (" + to_string(x) + ", " + to_string(y) + ") removed\n"));
                }//here the mutex is unlocked
                else {
                    send_to_client(client_fd, "Error: Invalid point format Use: Removepoint x,y\n");
                }
            } else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Removepoint x,y\n");
            }
        }
        else if (command == "CH") {
            // protect graph while computing hull
            lock_guard<std::mutex> lock(graph_mutex);// lock the mutex
            vector<Point> hull = grahamHull(points);// compute convex hull
            double area = polygonArea(hull);// compute area of the hull

            stringstream resp;// prepare response
            resp << "Convex Hull:\n";
            for (auto &p : hull) resp << "(" << p.x << ", " << p.y << ")\n";//loop through hull points and add to response
            resp.setf(ios::fixed); resp.precision(3);
            resp << "Area: " << area << "\n";

            string s = resp.str();// convert response to string
            send_to_client(client_fd, s.c_str());
        }//here the mutex is unlocked
        else if (command == "quit" || command == "exit") {
            send_to_client(client_fd, "bye!\n");
            close(client_fd);// close client socket
            return nullptr;//return nullptr to exit thread
        }
        else {
            send_to_client(client_fd, "Error: command isn't valid\n");
        }
    } 
    return nullptr;// should never reach here
}
        

int main() {

    // create listening socket
    const int PORT = 9034;// server port as we requested
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);// create TCP socket
    if (server_fd < 0) { perror("socket"); return 1; }

    // set up server address structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;// IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;// bind to all interfaces
    server_addr.sin_port = htons(PORT);// convert port to network byte order

    int a = 1; //integer to set the option
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(a));// set socket options to reuse address

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {// bind socket to address
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 18) < 0) {//18 is the maximum number of pending connections
        perror("listen");
        return 1;
    }

    cout << "Server listening on port 9034..." << endl;

    // using the proactor to handle clients from the library
    pthread_t proactorTid=startProactor(server_fd, client_thread);

    // wait for the proactor thread to finish
    pthread_join(proactorTid, nullptr);

    close(server_fd);

    return 0;
}