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
        if (line.empty()) continue;
        stringstream ss(line);
        string cmd;
        ss >> cmd;

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
            for(char &c : coords) if(c == ',') c = ' ';
            stringstream cs(coords);
            double x, y; 
            cs >> x >> y;
            newPoint(x, y);
        } 
        else if (cmd == "Removepoint") {
            string coords;
            ss >> coords; // Read the rest of the line from current stringstream
            for(char &c : coords) if(c == ',') c = ' ';
            stringstream cs(coords);
            double x, y; 
            cs >> x >> y;
            removePoint(x, y);
        }
    }
    return 0;
}