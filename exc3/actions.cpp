#include "actions.hpp"
using namespace std;


void newGraph(int n) {
    points.clear();
    for (int i = 0; i < n; i++) {
        double x, y;
        cin >> x >> y;
        points.push_back({x, y});
    }
}

void newPoint(double x, double y) {
    points.push_back({x, y});
}

void removePoint(double x, double y) {
    for (auto it = points.begin(); it != points.end(); ++it) {
        if (it->x == x && it->y == y) {
            points.erase(it);
            break;
        }
    }
}

void convexHull() {
    double area = 0.0; // Placeholder for area calculation if needed
    vector<Point> hull = grahamHull(points);

    area=polygonArea(hull);
    cout << "Convex Hull:" << endl;
    for (auto &p : hull) {
        cout << p.x << " " << p.y << endl;
    }

    cout << "Area: " << area << endl;
}