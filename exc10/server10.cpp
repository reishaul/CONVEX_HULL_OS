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
mutex graph_mutex;// Mutex for synchronizing graph operations
mutex graph_creation_mutex;// Mutex specifically for graph creation process
bool graph_initialized = false;// new
bool graph_being_created = false;// Flag to indicate graph is being created


// define posix variables for area monitoring
pthread_mutex_t area_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t area_condition = PTHREAD_COND_INITIALIZER;

//define shared variables for area monitoring
double current_area = 0.0;
bool area_changed = false;
bool area_at_least_100 = false;

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
* @brief Updates the monitored area and notifies if it crosses the threshold
* each time the graph changes we call this function
* @param area The new area value
*/
void update_area(double area) {
    pthread_mutex_lock(&area_mutex);// lock area mutex

    bool was_above_100 = area_at_least_100;
    area_at_least_100 = (area >= 100.0);

    current_area = area;

    area_changed = true;
    if (area_at_least_100 != was_above_100) {
        pthread_cond_broadcast(&area_condition);
        
    }
    pthread_mutex_unlock(&area_mutex);
}


void* area_state_thread(void*) {
    pthread_mutex_lock(&area_mutex);
    while (true) {
        while (!area_changed) {// wait for area change
            pthread_cond_wait(&area_condition, &area_mutex);
        }
        area_changed = false;

        if (area_at_least_100) {
            cout << "At Least 100 units belongs to CH (Current area: " << current_area << ")" << endl;
        
        } 
        else {
            cout << "At Least 100 units no longer belongs to CH (Current area: " << current_area << ")" << endl;
        }
    }
    pthread_mutex_unlock(&area_mutex);
    return nullptr;
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

        // Handle point input during graph creation
        if (awaiting_points) {// If waiting for points after Newgraph (to lock the graph when adding points)
            double x, y;
            stringstream ss(line);
            if (ss >> x >> y) {
                // Lock graph mutex for adding this specific point
                graph_mutex.lock();
                newPoint(x, y);
                points_needed--;// Decrement number of points still needed
                
                if (points_needed == 0) {
                    awaiting_points = false;
                    graph_being_created = false;
                    
                    // Calculate initial CH area after graph creation
                    vector<Point> hull = grahamHull(points);
                    double area = polygonArea(hull);
                    update_area(area);// Update area state
                    
                    graph_mutex.unlock();
                    // Unlock creation mutex - now other clients can access the completed graph
                    graph_creation_mutex.unlock();
                    
                    send_to_client(client_fd, "Graph created successfully\n");
                } else {
                    graph_mutex.unlock();
                    send_to_client(client_fd, "Point received. " + to_string(points_needed) + " more points needed.\n");
                }
            } else {
                send_to_client(client_fd, "Error: Invalid point format. Use: x y\n");
            }
            continue;
        }

        stringstream ss(line);// Create a stringstream from the input line call it ss
        string command;// Create a string to hold the command
        ss >> command;// Extract the command from the stringstream

        if (command == "Newgraph") {
            int n;//store number of points
            if (ss >> n && n > 0) {
                // Lock both mutexes for the entire graph creation process
                graph_creation_mutex.lock();
                graph_mutex.lock();
                
                initGraph();// Initialize empty graph
                graph_initialized = true;
                graph_being_created = true;
                
                // Unlock graph_mutex but keep graph_creation_mutex locked during point collection
                graph_mutex.unlock();
                
                awaiting_points = true;
                points_needed = n;
                send_to_client(client_fd, ("send " + to_string(n) + " points\n").c_str());
            }
            else {
                send_to_client(client_fd, "Invalid number of points\n");
            }
        }
        else if (command == "Newpoint") {

            // Check if graph exists before adding points
            {
                lock_guard<mutex> lock(graph_mutex);
                if (!graph_initialized) {
                    send_to_client(client_fd, "Error: No graph exists. Create a graph first with 'Newgraph <n>'\n");
                    continue;
                }
            }
            string coords;
            if (ss >> coords) {
                for (char &c : coords) if (c == ',') c = ' ';// replace commas with space
                stringstream cs(coords);// stringstream for parsing point call it cs
                double x, y;

                if (cs >> x >> y) {
                    lock_guard<mutex> lock(graph_mutex);// lock the mutex
                    newPoint(x, y);
                    send_to_client(client_fd, ("Point (" +to_string(x) + ", " + to_string(y) + ") added\n"));

                    vector<Point> hull = grahamHull(points);// compute convex hull
                    double area = polygonArea(hull);// compute area of the hull
                    update_area(area);
                }//here the mutex is unlocked
                else {
                    send_to_client(client_fd, "Error: Invalid point format Use: Newpoint x,y\n");
                }
            } else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Newpoint x,y\n");
            }
        }
        else if (command == "Removepoint") {

            // Check if graph exists before removing points
            {
                lock_guard<mutex> lock(graph_mutex);
                if (!graph_initialized) {
                    send_to_client(client_fd, "Error: No graph exists. Create a graph first with 'Newgraph <n>'\n");
                    continue;
                }
            }

            string coords;
            if (ss >> coords) {
                for (char &c : coords) if (c == ',') c = ' ';// replace commas with space
                stringstream cs(coords);// stringstream for parsing point call it cs
                double x, y;
                if (cs >> x >> y) {
                    lock_guard<mutex> lock(graph_mutex);// lock the mutex 
                    removePoint(x, y);
                    send_to_client(client_fd, ("Point (" + to_string(x) + ", " + to_string(y) + ") removed\n"));

                    vector<Point> hull = grahamHull(points);// compute convex hull
                    double area = polygonArea(hull);// compute area of the hull
                    update_area(area);
                }//here the mutex is unlocked
                else {
                    send_to_client(client_fd, "Error: Invalid point format Use: Removepoint x,y\n");
                }
            } else {
                send_to_client(client_fd, "Error: No coordinates provided Use: Removepoint x,y\n");
            }
        }
        else if (command == "CH") {
            // Wait for graph creation to complete if in progress
            {
                lock_guard<std::mutex> creation_lock(graph_creation_mutex);
                // This will block if graph is being created
            }
            
            // Now protect graph while computing hull
            lock_guard<std::mutex> lock(graph_mutex);// lock the mutex

            if (!graph_initialized) {// Check if graph exists before computing CH
                send_to_client(client_fd, "Error: No graph exists. Create a graph first with 'Newgraph <n>'\n");
                continue;
            }
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

    // start area state thread
    pthread_t state_tid;
    if (pthread_create(&state_tid, nullptr, area_state_thread, nullptr) != 0) {
        perror("pthread_create");
        return 1;
    }

    pthread_detach(state_tid); // Detach(לנתק) the state thread

    // using the proactor to handle clients from the library
    pthread_t proactorTid=startProactor(server_fd, client_thread);

    // wait for the proactor thread to finish
    pthread_join(proactorTid, nullptr);

    pthread_mutex_destroy(&area_mutex);
    pthread_cond_destroy(&area_condition);

    close(server_fd);

    return 0;
}