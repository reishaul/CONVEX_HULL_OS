// #include "actions.hpp"
// using namespace std;

// /*
//  * Create a new graph with n points
//  @param n: number of points
//  */
// void newGraph(int n) {
//     points.clear();// Clear existing points
//     for (int i = 0; i < n; i++) {
//         double x, y;
//         cin >> x >> y;
//         points.push_back({x, y});
//     }
// }

// /*
//  * Add a new point to the current graph
//  @param x: x coordinate of the point
//  @param y: y coordinate of the point
//  */
// void newPoint(double x, double y) {
//     points.push_back({x, y});
// }

// /*
//  * Remove a point from the current graph
//  @param x: x coordinate of the point
//  @param y: y coordinate of the point
//  */
// void removePoint(double x, double y) {
//     for (auto it = points.begin(); it != points.end(); ++it) {
//         if (it->x == x && it->y == y) {
//             points.erase(it);
//             break;
//         }
//     }
// }

// /*
//  * Compute and print the convex hull of the current graph
//  */
// void convexHull() {
//     double area = 0.0; // Placeholder for area calculation if needed
//     vector<Point> hull = grahamHull(points);

//     area=polygonArea(hull);//get the area of the polygon
//     cout << "Convex Hull:" << endl;
//     for (auto &p : hull) {//go over all the points in the hull
//         cout << p.x << " " << p.y << endl;
//     }

//     cout << "Area: " << area << endl;
// }