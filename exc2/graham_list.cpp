#include "graham_common.hpp"
#include <algorithm>
#include <cmath>
#include <list>

//Computes the cross product of vectors OA and OB
double cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x)*(B.y - O.y) - (A.y - O.y)*(B.x - O.x);
}

// Computes the squared distance between points A and B
double dist2(const Point& a, const Point& b) {
    double dx = a.x - b.x, dy = a.y - b.y;
    return dx*dx + dy*dy;
}

// Computes the convex hull of a set of 2D points using Graham's scan algorithm
// Returns the vertices of the convex hull in counter-clockwise order
vector<Point> grahamHull(vector<Point> p) {
    int n = (int)p.size();
    if (n <= 1) return p;

    int pivot = 0;
    for (int i = 1; i < n; i++) {
        if (p[i].y < p[pivot].y || (p[i].y == p[pivot].y && p[i].x < p[pivot].x)) //
            pivot = i;
    }
    
    swap(p[0], p[pivot]);
    Point O = p[0];

    sort(p.begin() + 1, p.end(), [&](const Point& a, const Point& b){
        double c = cross(O, a, b);
        if (c != 0) return c > 0;
        return dist2(O, a) < dist2(O, b);
    });

    // Now using list instead of vector for the stack
    list<Point> st;
    st.push_back(p[0]);
    for (int i = 1; i < n; i++) {
        while (st.size() >= 2) {
            auto it_last = prev(st.end());
            auto it_second = prev(it_last);   
            if (cross(*it_second, *it_last, p[i]) <= 0)
                st.pop_back();
            else
                break;
        }
        st.push_back(p[i]);
    }
    // Convert deque back to vector before returning to match function signature
    return vector<Point>(st.begin(), st.end());
}

// Computes the area of a polygon given its vertices using the shoelace formula
double polygonArea(const vector<Point>& P) {
    int m = (int)P.size();
    double s = 0;
    for (int i = 0; i < m; i++) {
        int j = (i + 1) % m;
        s += (double)P[i].x * P[j].y - (double)P[i].y * P[j].x;
    }
    return (s) * 0.5;
}
