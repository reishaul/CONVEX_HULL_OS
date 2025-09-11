#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sstream>
#include <netdb.h>

#include "../exc3/actions.hpp" // Include the actions header file
#include <map>


// Constants
#define PORT "9034"
#define BACKLOG 12//how many pending connections the queue will hold
#define BUFFER_SIZE 1024// size of the buffer for reading and writing
#define MAX_CLIENTS 25

using namespace std;

void send_to_client(int client_fd, const string &msg) {
    send(client_fd, msg.c_str(), msg.size(), 0);
}

struct ClientState {
    bool awaiting_points = false;
    int points_needed = 0;
};

map<int, ClientState> client_states; // map socket_fd -> state

// Global flag to track if the graph is being modified or calculated
bool graph_in_use = false;
vector<int> waiting_clients; // clients waiting for the graph to be free

bool handle_command(int client_fd, const string &cmd, fd_set *list) {
    if(cmd.empty()) return false;

    if(client_states[client_fd].awaiting_points) {
        double x, y;
        stringstream ss(cmd);
        if(ss >> x >> y){
            newPoint(x, y);
            client_states[client_fd].points_needed--;

            if(client_states[client_fd].points_needed == 0) {
                client_states[client_fd].awaiting_points = false;
                graph_in_use = false; // Release the graph
                // Notify waiting clients
                send_to_client(client_fd, "Graph created\n");

                for(int waiting_fd : waiting_clients) {
                    send_to_client(waiting_fd, "Graph is now free. You can proceed with your command\n");
                }
                waiting_clients.clear();
            }
        }
        else {
            send_to_client(client_fd, "Error: Invalid point format Use: x y\n");
        }
        return false;

    }
    stringstream ss(cmd);// Create a stringstream from the input line call it ss
    string command;// Create a string to hold the command
    ss >> command;

    // Commands that modify or calculate the graph - check if graph is busy
    if(command == "Newgraph" || command == "Newpoint" || command == "Removepoint" || command == "CH") {
        if(graph_in_use) {
            waiting_clients.push_back(client_fd);
            send_to_client(client_fd, "Graph is currently being modified. Please wait...\n");
            return false;
        }
    }

    if(command == "Newgraph") {
        int n;
        if(ss >> n && n > 0) {
            graph_in_use = true; // Mark the graph as in use

            client_states[client_fd].awaiting_points = true;
            client_states[client_fd].points_needed = n;
            initGraph();
            send_to_client(client_fd, "send " + to_string(n) + " points\n");
        } 
        else {
            send_to_client(client_fd, "Invalid number of points\n");
        }
    }

    else if(command == "CH") {

        graph_in_use = true; // Mark the graph as in use
        stringstream response;
        vector<Point> hull = grahamHull(points);
        double area = polygonArea(hull);
        response << "Convex Hull:\n";
        for (auto &p : hull) {
            response << "(" << p.x << ", " << p.y << ")\n";
        }
        response.setf(ios::fixed);// Fixed point notation
        response.precision(3);// Set precision for area output
        response << "Area: " << area << "\n";
        send_to_client(client_fd, response.str());// Send the response to the client. response.str() converts the stringstream to a string
        graph_in_use = false; // Release the graph
        // Notify waiting clients   

        for(int waiting_fd : waiting_clients) {
            send_to_client(waiting_fd, "Graph is now free. You can proceed with your command\n");
        }
        waiting_clients.clear();
    }
    else if(command == "Newpoint") {
        string coords;
        if(ss >> coords){
            graph_in_use = true; // Mark the graph as in use
        
            for(char &c : coords) if(c == ',') c = ' ';//replace , with space
            stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
            double x, y; 
            if(cs >> x >> y){
                newPoint(x, y);
                send_to_client(client_fd, "Point (" + to_string(x) + ", " + to_string(y) + ") added\n");
            }
            else {
                send_to_client(client_fd, "Error: Invalid point format Use: x,y\n");
            }
            graph_in_use = false; // Release the graph

            // Notify waiting clients   
            for(int waiting_fd : waiting_clients) {
                send_to_client(waiting_fd, "Graph is now free. You can proceed with your command\n");
            }
            waiting_clients.clear();
        }
        else {
            send_to_client(client_fd, "Error: No coordinates provided Use: Newpoint x,y\n");
        }
    } 
    else if(command == "Removepoint") {
        string coords;
        if(ss >> coords){
            graph_in_use = true; // Mark the graph as in use
        
            for(char &c : coords) if(c == ',') c = ' ';//replace , with space
            stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
            double x, y; 
            if(cs >> x >> y){
                removePoint(x, y);
                send_to_client(client_fd, "Point (" + to_string(x) + ", " + to_string(y) + ") removed\n");
            }
            else {
                send_to_client(client_fd, "Error: Invalid point format Use: x,y\n");
            }
            graph_in_use = false; // Release the graph

            // Notify waiting clients   
            for(int waiting_fd : waiting_clients) {
                send_to_client(waiting_fd, "Graph is now free. You can proceed with your command\n");
            }
            waiting_clients.clear();
        }
        else {
            send_to_client(client_fd, "Error: No coordinates provided Use: Removepoint x,y\n");
        }
    }

    else if (command == "quit" || command == "exit") {// Handle client disconnection
        send_to_client(client_fd, "bye!\n");
        close(client_fd);
        FD_CLR(client_fd, list);// remove from the list of sockets

        waiting_clients.erase(remove(waiting_clients.begin(), waiting_clients.end(), client_fd), waiting_clients.end());
        client_states.erase(client_fd);
        return true; // Connection closed
    }

    else {
        send_to_client(client_fd, "Error: command isn't valid\n");
    }
    return false; // Connection still active
}

int main(int argc, char *argv[]) {

    //some declarations
    fd_set read_fds; // set of sockets for reading
    fd_set list;

    int id; // to count the number of clients
    int max_fd;
    int newfd;

    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    int byte_count;
    char buffer[BUFFER_SIZE]; // buffer for reading and writing

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&list);
    FD_ZERO(&read_fds);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, PORT, &hints, &ai)!=0) {
        perror("getaddrinfo error");
        return 1;
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        id = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (id < 0) continue;

        int yes=1;
        setsockopt(id, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(id, p->ai_addr, p->ai_addrlen) < 0) {
            close(id);
            continue;
        }
        break;
    }

    if(p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    freeaddrinfo(ai);
    if(listen(id, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    FD_SET(id, &list);
    max_fd = id;

    cout << "Server listening on port " << PORT << "...\n";

    // main loop to accept and handle client connections
    while (true) {

        read_fds = list; // copy the list of sockets
        if (select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
            perror("select");
            exit(4);
        }

        for(int i=0; i<=max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) { // check if the socket is ready
                if (i == id) { // new connection

                    addrlen = sizeof remoteaddr;
                    newfd = accept(id, (struct sockaddr *)&remoteaddr, &addrlen);// accept the new connection

                    if (newfd == -1) {// if accept failed
                        perror("accept");
                    } 
                    else {// if accept succeeded
                        FD_SET(newfd, &list); // add to the list of sockets
                        if (newfd > max_fd) {// update the maximum socket number
                            max_fd = newfd;
                        }
                        cout<<"new connection on socket "<<newfd<<"\n";
                        client_states[newfd] = ClientState(); // Initialize client state

                        send_to_client(newfd, "Welcome to the Convex Hull Server! (quit to end)\n");
                    }
                } 
                else { // handle data from a client
                    memset(buffer, 0, sizeof buffer);
                    if ((byte_count = recv(i, buffer, sizeof buffer, 0)) <= 0) {// if recv failed or connection closed
                        if (byte_count == 0) {
                            cout << "Socket " << i << " disconnected\n";
                        } 
                        else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &list); // remove from the list of sockets

                        client_states.erase(i);
                        waiting_clients.erase(remove(waiting_clients.begin(), waiting_clients.end(), i), waiting_clients.end());
                    } 
                    else {
                        string line(buffer);

                        if(!line.empty() && line.back() == '\n') {
                            line.pop_back(); // remove trailing newline
                        }

                        if(!line.empty() && line.back() == '\r') {//if there is a carriage return
                            line.pop_back(); // remove trailing carriage return
                        }

                        cout<<"Received from socket "<<i<<": "<<line<<"\n";

                        bool connection_closed = handle_command(i, line, &list);
                        if (connection_closed) {
                            continue; // Skip further processing for this socket
                        }
                    }
                }
            }
        }
    }
    return 0;
}