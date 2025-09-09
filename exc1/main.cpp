#include <iostream>
#include <sstream>
#include "graham.hpp"
using namespace std;


/*
 * Main function to drive the program
 */
int main() {
    int n;//number of input points
    cout << "Enter number of points:\n";
    if (!(cin >> n)|| n <= 2) {
        cout << "Invalid input-> Exiting\n";
        return 0;
    }
    string line;
    getline(cin, line); // consume newline after number

    vector<Point> pts;//vector to store input points
    pts.reserve(n);//reserve space for n points
    cout << "Start entering points in format x,y:\n";

    for (int i = 0; i < n; i++) {//read n points
        getline(cin, line);      // line in format x,y

        for (char& c : line)     // replace "," with space
            if (c == ',') c = ' ';

        stringstream ss(line);
        Point p;

        if(!(ss >> p.x >> p.y)) {
            cout << "Invalid point format: " <<(i+1)
                 << " expected x,y (e.g. 1.4,2)\n";
            return 1;
        }
        ss >> p.x >> p.y;
        pts.push_back(p);//add point to vector
    }

    vector<Point> hull = grahamHull(pts);//compute convex hull
    double area = polygonArea(hull);//compute area of convex hull

    cout.setf(ios::fixed);//always show the numbers in a regular format with decimal point
    cout.precision(3); // set precision to 3 decimal places
    cout<<"The area of the convex hull is: " << area << "\n";

    return 0;
}
