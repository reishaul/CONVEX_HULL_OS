#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "actions.cpp" // Include the actions header file
using namespace std;


int main() {
    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;// Skip empty lines
        stringstream ss(line);// Create a stringstream from the input line call it ss
        string cmd;// Create a string to hold the command
        ss >> cmd;

        //command handling and calling the relevant functions
        if (cmd == "Newgraph") {
            int n; 
            ss >> n;
            newGraph(n);
        } 
        else if (cmd == "CH") {
            convexHull();
        } 
        else if (cmd == "Newpoint") {
            string coords; //hold the rest of the line the cordinates
            ss >> coords; // Read the rest of the line from current stringstream
            for(char &c : coords) if(c == ',') c = ' ';//replace , with space
            stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
            double x, y; 
            cs >> x >> y;
            newPoint(x, y);
        } 
        else if (cmd == "Removepoint") {
            string coords;
            ss >> coords; // Read the rest of the line from current stringstream
            for(char &c : coords) if(c == ',') c = ' ';//replace , with space
            stringstream cs(coords);// Create a new stringstream to parse the coordinates call it cs
            double x, y; 
            cs >> x >> y;
            removePoint(x, y);
        }
    }
    return 0;
}