#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>

#include <cstring>


#include <thread>
#include <mutex>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../exc3/actions.hpp" // Include the actions header file
#include <map>//include map for client states
#include <sstream>


// Constants
#define PORT "9034"
#define BACKLOG 12//how many pending connections the queue will hold
#define BUFFER_SIZE 1024// size of the buffer for reading and writing

using namespace std;

std::mutex graph_mutex;// Mutex to protect access to the graph

/*
@brief Sends a message to the specified client socket
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



// Thread function handles a single client
void client_thread_func(int client_fd) {

    cout<<"new client on socket "<<client_fd<<" thread id: "<<this_thread::get_id()<<"\n";//print thread id and socket id
    char buffer[BUFFER_SIZE];// buffer for reading and writing

    // Local client state (no global tracking necessary because each client has its own thread)
    struct ClientStateLocal {
        bool awaiting_points = false;
        int points_needed = 0;
    } state;

    send_to_client(client_fd, "Welcome to the Convex Hull Server! (quit to end)\n");

    while (true) {//loop to handle client commands
        memset(buffer, 0, sizeof(buffer));// clear the buffer
        ssize_t byte_count = recv(client_fd, buffer, sizeof(buffer) - 1, 0);//store number of bytes read
        if (byte_count <= 0) {// if recv failed or connection closed
            if (byte_count == 0) {
                cout << "Socket " << client_fd << " disconnected\n";
            } else {
                perror("recv");
            }
            close(client_fd);
            return;
        }

        string line(buffer);// Convert buffer to string
        
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();// trim newline/cr (lefties in the line)

        if (line.empty()) continue;

        cout << "Received from socket " << client_fd << ": " << line << "\n";

        if(state.awaiting_points) {// If waiting for points after Newgraph

            stringstream ss(line);// Create a stringstream from the command to parse the point call it ss
            double x, y;
            if (ss >> x >> y) {//if successfully parsed two doubles
                {
                    std::lock_guard<std::mutex> lock(graph_mutex);// Lock the graph for exclusive access
                    newPoint(x, y);
                }
                state.points_needed--;// Decrement the number of points still needed
                if (state.points_needed == 0) {// If all points have been received
                    state.awaiting_points = false;// define that we are no longer waiting for points
                    send_to_client(client_fd, "Graph created\n");
                }
            }
            else{// If parsing failed
                send_to_client(client_fd, "Error: Invalid point format Use: x y\n");
            }
            continue;

        }
        stringstream ss(line);// Create a stringstream from the input line call it ss
        string command;// Create a string to hold the command
        ss >> command;// Extract the command from the stringstream

        if(command == "Newgraph") {
            int n;
            if(ss >> n && n > 0) {// If successfully parsed a positive integer
                {
                    std::lock_guard<std::mutex> lock(graph_mutex);
                    initGraph();// Initialize empty graph
                }
                state.awaiting_points = true;// Expecting points next
                state.points_needed = n;//define how many points are needed
                send_to_client(client_fd, "send " + to_string(n) + " points\n");
            } 
            else {
                send_to_client(client_fd, "Invalid number of points\n");
            }
        }
        else if(command == "CH") {
            //Compute convex hull and area while holding the graph mutex
            std::lock_guard<std::mutex> lock(graph_mutex);// Lock the graph for exclusive access
            vector<Point> hull = grahamHull(points);//vector of points that are in the convex hull
            double area = polygonArea(hull);// calculate area of the convex hull

            stringstream response;
            response << "Convex Hull:\n";
            for (auto &p : hull) {// iterate through points in the convex hull
                response << "(" << p.x << ", " << p.y << ")\n";
            }
            response.setf(ios::fixed);// Fixed point notation
            response.precision(3);// Set precision for area output 3 places after decimal
            response << "Area: " << area << "\n";
            send_to_client(client_fd, response.str());// send response to client
        }

        else if(command == "Newpoint") {
            string coords;
            if(ss >> coords){// If coordinates were provided

                //graph_mutex.lock();// Lock the graph for exclusive access
                for(char &c : coords) if(c == ',') c = ' ';//replace , with space
                stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
                double x, y; 
                if(cs >> x >> y){
                    lock_guard<mutex> lock(graph_mutex);// Lock the graph for exclusive access
                    newPoint(x, y);
                    send_to_client(client_fd, "Point (" + to_string(x) + ", " + to_string(y) + ") added\n");// send confirmation to client
                }
                else {// If parsing failed
                    send_to_client(client_fd, "Error: Invalid point format Use: x,y\n");
                }

            }
            else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Newpoint x,y\n");// send error message to client
            }
        }
        else if(command == "Removepoint") {
            string coords;// coordinates to remove
            if(ss >> coords){
                //graph_mutex.lock();// Lock the graph for exclusive access
                for(char &c : coords) if(c == ',') c = ' ';//replace , with space
                stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
                double x, y; // coordinates to remove
                if(cs >> x >> y){
                    lock_guard<mutex> lock(graph_mutex);
                    removePoint(x, y);// Remove the point from the graph using the provided coordinates
                    send_to_client(client_fd, "Point (" + to_string(x) + ", " + to_string(y) + ") removed\n");
                }
                else {// If parsing failed
                    send_to_client(client_fd, "Error: Invalid point format Use: x,y\n");
                }

            }
            else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Removepoint x,y\n");
            }
        }
        //option to quit the connection
        else if (command == "quit" || command == "exit") {// Handle client disconnection
            send_to_client(client_fd, "bye!\n");
            close(client_fd);//close the socket
            return; // Exit the thread
        }
        else{// Invalid command
            send_to_client(client_fd, "Error: command isn't valid\n");
        }
    }
}
/*
 * @brief Handles a command from a client
 * @param client_fd File descriptor for the client socket
 * @param line The command line received from the client
 * @param list Pointer to the master list of sockets (for removing on quit)
 * @return true if the connection should be closed, false otherwise
 */
int main(int argc, char *argv[]) {

    struct addrinfo hints, *ai, *p;// hints for getaddrinfo that will point to the results
    int listen_fd;// listening socket file descriptor


    memset(&hints, 0, sizeof hints);// zero out the hints structure
    hints.ai_family = AF_UNSPEC;// don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;// tcp stream sockets
    hints.ai_flags = AI_PASSIVE;// fill in my IP for me

    if(getaddrinfo(NULL, PORT, &hints, &ai)!=0) {// get the address info
        perror("getaddrinfo error");// print error message
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = ai; p != NULL; p = p->ai_next) {
       listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);// create a socket by calling socket function
        if (listen_fd < 0) continue;

        int yes=1;// lose the pesky "address already in use" error message
        if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {//allow reuse of local addresses
            perror("setsockopt");
            close(listen_fd);
            continue;
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) < 0) {// if bind fails
            close(listen_fd);
            continue;
        }
        break;
    }

    if(p == NULL) {// if we got here, it means we didn't bind successfully
        fprintf(stderr, "server: failed to bind\n");
        exit(1);//exit code 1
    }

    freeaddrinfo(ai); // all done with this structure

    if (listen(listen_fd, BACKLOG) == -1) {// start listening for incoming connections
        perror("listen");
        exit(1);
    }

    cout<<"Server listening on port "<<PORT<<"...\n";


    // main loop to accept and handle client connections 
    while (true) {

        struct sockaddr_storage remoteaddr; // client address
        socklen_t addrlen = sizeof remoteaddr;// length of client address
        int newfd = accept(listen_fd, (struct sockaddr *)&remoteaddr, &addrlen);// accept the new connection
        if (newfd == -1) {// if accept failed
            perror("accept");
            continue;
        }
        cout<<"new connection on socket "<<newfd<<"\n";

        try{// try to start a new thread for the client
            thread t(client_thread_func, newfd);// Start a new thread for the client and detach it
            t.detach();
        }
        catch(...) {// If thread creation failed we catch all exceptions
            perror("Thread creation failed");
            close(newfd);
        }
    }

    close(listen_fd);//close the listening socket (unreachable code in this example)

    return 0;
}