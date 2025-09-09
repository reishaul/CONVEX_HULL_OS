#include <iostream>
#include <sstream>
#include "graham_common.hpp"
using namespace std;



int main() {
    int n;
    cout << "Enter number of points:\n";
    if (!(cin >> n)|| n <= 2) {
        cout << "Invalid input-> Exiting.\n";
        return 0;
    }
    string line;
    getline(cin, line); // consume newline after number

    vector<Point> pts;// vector to hold the points
    pts.reserve(n);// optional, to avoid multiple allocations

    cout << "Start entering points in format x,y:\n";
    for (int i = 0; i < n; i++) {
        getline(cin, line);      // line in format x,y
        for (char& c : line)     // replace "," with space
            if (c == ',') c = ' ';
        stringstream ss(line);
        Point p;
        ss >> p.x >> p.y;
        pts.push_back(p);
    }

    vector<Point> hull = grahamHull(pts);
    double area = polygonArea(hull);

    cout.setf(ios::fixed);
    cout.precision(3); //
    cout<<"The area of the convex hull is: " << area << "\n";

    return 0;
}
