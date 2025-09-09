#ifndef GRAHAM_HPP
#define GRAHAM_HPP

#include <vector>
using namespace std;


struct Point {
    double x, y;
    };

double cross(const Point& O, const Point& A, const Point& B);

double dist2(const Point& a, const Point& b);

vector<Point> grahamHull(vector<Point> p);

double polygonArea(const vector<Point>& P);

#endif 
