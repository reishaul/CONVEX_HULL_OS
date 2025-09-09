#include "graham.hpp"
#include <algorithm>
#include <cmath>

/*
 * Computes the cross product of vectors OA and OB
 * A positive cross product indicates a counter-clockwise turn,
 * a negative cross product indicates a clockwise turn,
 * and a zero cross product indicates collinear points.
 */
double cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x)*(B.y - O.y) - (A.y - O.y)*(B.x - O.x);
}

/*
 * Computes the squared distance between points A and B
 */
double dist2(const Point& a, const Point& b) {
    double dx = a.x - b.x, dy = a.y - b.y;
    return dx*dx + dy*dy;
}

/*
 * Computes the convex hull of a set of 2D points using Graham's scan algorithm
 * Returns the vertices of the convex hull in counter-clockwise order
 */
vector<Point> grahamHull(vector<Point> p) {
    int n = (int)p.size();//store size of input vector
    if (n <= 1) return p;//if size is 1 or less, return input vector

    int pivot = 0;//index of point with lowest y-coordinate (and leftmost in case of tie)


    for (int i = 1; i < n; i++) {//find pivot point
        if (p[i].y < p[pivot].y || (p[i].y == p[pivot].y && p[i].x < p[pivot].x)) 
            pivot = i;
    }

    swap(p[0], p[pivot]);//swap pivot point with first point
    Point O = p[0];

    // Sort points by polar angle with pivot, using cross product for orientation
    // In case of tie (collinear points), the closer point comes first
    // This ensures that the convex hull is constructed correctly
    sort(p.begin() + 1, p.end(), [&](const Point& a, const Point& b){
        double c = cross(O, a, b);
        if (c != 0) return c > 0;
        return dist2(O, a) < dist2(O, b);
    });

    // Build the convex hull using a vector as a stack
    vector<Point> st;
    st.reserve(n);
    st.push_back(p[0]);
    for (int i = 1; i < n; i++) {//iterate through sorted points
        // Remove points from the stack while we have a non-left turn
        while (st.size() >= 2 && cross(st[st.size()-2], st.back(), p[i]) <= 0)
            st.pop_back();
        st.push_back(p[i]);
    }
    return st;
}

/*
 * Computes the area of a polygon given its vertices
 * Uses the shoelace formula to compute the area
 */
double polygonArea(const vector<Point>& P) {
    int m = (int)P.size();
    double s = 0;
    for (int i = 0; i < m; i++) {
        int j = (i + 1) % m;
        s += (double)P[i].x * P[j].y - (double)P[i].y * P[j].x;
    }
    return (s) * 0.5;
}
