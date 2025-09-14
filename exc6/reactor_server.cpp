#include <iostream>
#include <netinet/in.h>

#include <sys/socket.h>

#include <unistd.h>
#include <map>
#include <vector>

#include <sstream>

#include "../exc3/actions.hpp" // Include the actions header file
#include "../exc5/reactor.hpp"


// Constants
#define PORT 9034
#define BACKLOG 12//how many pending connections the queue will hold
#define BUFFER_SIZE 1024// size of the buffer for reading and writing

void* globalReactor = nullptr;

using namespace std;

/*
@brief Represents the state of a connected client   
*/
struct ClientState {
    bool awaiting_points = false;//waiting for points after Newgraph
    int points_needed = 0;//number of points still needed
};


map<int, ClientState> client_states; // map that associates each client socket fd with its state

// Global flag to track if the graph is being modified or calculated
bool graph_in_use = false;
vector<int> waiting_clients; // clients waiting for the graph to be free


/*
@brief Sends a message to the specified client socket
@param client_fd The file descriptor of the client socket
@param msg The message to send
*/
void send_to_client(int client_fd, const string &msg) {
    send(client_fd, msg.c_str(), msg.size(), 0);
}

/*
@brief Handles a command from a client
@param client_fd The file descriptor of the client socket   
@param cmd The command string received from the client
@param list Pointer to the set of active client sockets
@return true if the client connection should be closed, false otherwise
*/
bool handle_command(int client_fd, const string &cmd) {
    if(cmd.empty()) return false;// Ignore empty commands

    if(client_states[client_fd].awaiting_points) {// If waiting for points after Newgraph
        double x, y;// coordinates of the new point
        stringstream ss(cmd);// Create a stringstream from the command to parse the point
        if(ss >> x >> y){// If successfully parsed two doubles
            newPoint(x, y);// Add the new point to the graph
            client_states[client_fd].points_needed--;// Decrement the number of points still needed

            if(client_states[client_fd].points_needed == 0) {// If all points have been received
                client_states[client_fd].awaiting_points = false;
                graph_in_use = false; // Release the graph
                // Notify waiting clients
                send_to_client(client_fd, "Graph created\n");// Acknowledge graph creation

                for(int waiting_fd : waiting_clients) {// Notify all waiting clients that the graph is free to changes
                    send_to_client(waiting_fd, "Graph is now free. You can proceed with your command\n");
                }
                waiting_clients.clear();// Clear the waiting list
            }
        }
        else {// If parsing failed
            send_to_client(client_fd, "Error: Invalid point format Use: x y\n");
        }
        return false;// Connection still active

    }
    stringstream ss(cmd);// Create a stringstream from the input line call it ss
    string command;// Create a string to hold the command
    ss >> command;// Extract the command from the stringstream. ss is the stringstream object created from the command string

    // Commands that modify or calculate the graph - check if graph is busy
    if(command == "Newgraph" || command == "Newpoint" || command == "Removepoint" || command == "CH") {
        if(graph_in_use) {// If the graph is currently being modified or calculated
            waiting_clients.push_back(client_fd);
            send_to_client(client_fd, "Graph is currently being modified. Please wait...\n");
            return false;
        }
    }

    if(command == "Newgraph") {
        int n;
        if(ss >> n && n > 0) {// If successfully parsed a positive integer
            graph_in_use = true; // Mark the graph as in use

            client_states[client_fd].awaiting_points = true;// Expecting points next
            client_states[client_fd].points_needed = n;//
            initGraph();// Initialize empty graph
            send_to_client(client_fd, "send " + to_string(n) + " points\n");
        } 
        else {
            send_to_client(client_fd, "Invalid number of points\n");
        }
    }

    else if(command == "CH") {

        graph_in_use = true; // Mark the graph as in use
        stringstream response;// String stream to build the response
        vector<Point> hull = grahamHull(points);//vector of points that are in the convex hull
        double area = polygonArea(hull);
        response << "Convex Hull:\n";
        for (auto &p : hull) {
            response << "(" << p.x << ", " << p.y << ")\n";
        }
        response.setf(ios::fixed);// Fixed point notation
        response.precision(3);// Set precision for area output
        response << "Area: " << area << "\n";
        send_to_client(client_fd, response.str());// Send the response to the client. response.str() converts the stringstream to a string
        graph_in_use = false; // free the graph

        // Notify waiting clients   
        for(int waiting_fd : waiting_clients) {
            send_to_client(waiting_fd, "Graph is now free. You can proceed with your command\n");
        }
        waiting_clients.clear();
    }

    else if(command == "Newpoint") {
        string coords;
        if(ss >> coords){// If coordinates were provided
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
        string coords;// coordinates to remove
        if(ss >> coords){
            graph_in_use = true; // Mark the graph as in use
        
            for(char &c : coords) if(c == ',') c = ' ';//replace , with space
            stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
            double x, y; // coordinates to remove
            if(cs >> x >> y){
                removePoint(x, y);// Remove the point from the graph using the provided coordinates
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

    //option to quit the connection
    else if (command == "quit" || command == "exit") {// Handle client disconnection
        send_to_client(client_fd, "bye!\n");
        return true; // Connection closed
    }
    //     close(client_fd);//close the socket
    //     removeFdFromReactor(globalReactor, client_fd);// remove from the reactor's master set

    //     waiting_clients.erase(remove(waiting_clients.begin(), waiting_clients.end(), client_fd), waiting_clients.end());
    //     client_states.erase(client_fd);// Remove client state
    //     return true; // Connection closed
    // }

    else {
        send_to_client(client_fd, "Error: command isn't valid\n");
    }
    return false; // Connection still active
}

/*
 * @brief Handles communication with a client
 * @param client_fd File descriptor for the client socket
 * @return nullptr
 */
void* clientHandler(int client_fd) {
    char buffer[BUFFER_SIZE]; // buffer for reading and writing
    int byte_count= recv(client_fd, buffer, sizeof(buffer), 0);// number of bytes read

    if (byte_count <= 0) {// if recv failed or connection closed
        cout << "Socket " << client_fd << " disconnected\n";
        close(client_fd);
        client_states.erase(client_fd);// Remove client state
        return nullptr;
    }

    string line(buffer, byte_count);// Convert buffer to string

    if(!line.empty() && (line.back() == '\n' || line.back() == '\r')) {//if there is a newline
            line.pop_back(); // remove trailing newline
        }

    cout<<"Received "<<line <<endl;
    bool should_close = handle_command(client_fd, line);
    
    // If the command indicated we should close, just close the socket
    // The reactor will detect this on the next iteration
    if (should_close) {
        close(client_fd);
        client_states.erase(client_fd);
    }
    
    return nullptr;
}

/*
 * @brief Handles incoming client connections
 * @param listener_fd File descriptor for the listening socket
 * @return nullptr
 */
void* listenerHandler(int listener_fd) {
    struct sockaddr_storage clientAddr; // client address
    socklen_t addrlen = sizeof clientAddr;// length of client address

    int newfd = accept(listener_fd, (sockaddr *)&clientAddr, &addrlen);// accept the new connection

    if (newfd == -1) {// if accept failed
        perror("accept");
        return nullptr;
    } 

    cout<<"Accepted new connection on socket "<<newfd<<"\n";
    client_states[newfd] = ClientState(); // Initialize client state
    send_to_client(newfd, "Welcome to the Convex Hull Server! (quit to end)\n");

    addFdToReactor(globalReactor, newfd, clientHandler);// add the new client socket to the reactor
    return nullptr;
}




int main() {

    int listener = socket(AF_INET, SOCK_STREAM, 0);// create a socket by calling socket function
    int yes=1;// lose the pesky "address already in use" error message
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));//allow reuse of local addresses

    sockaddr_in server_addr{};// server address structure
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr = in_addr{}; // Any local address
    server_addr.sin_port = htons(PORT); // Port number

    if (bind(listener, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {// if bind fails
        perror("bind");
        return 1;
    }

    if(listen(listener, BACKLOG) < 0) {// start listening on the socket
        perror("listen");
        return 1;
    }
    cout << "Server listening on port " << PORT << "...\n";

    globalReactor=startReactor();//start the reactor that runs with select in a separate thread
    addFdToReactor(globalReactor, listener, listenerHandler);// Add the listener socket to the reactor

    while(true){
        sleep(1);// Keep the main thread alive
    }
}
