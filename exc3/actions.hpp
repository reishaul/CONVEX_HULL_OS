#ifndef ACTION_HPP
#define ACTION_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "../exc2/graham_common.hpp"
using namespace std;

vector<Point> points;

void newGraph(int n);

void newPoint(double x, double y);

void removePoint(double x, double y);

void convexHull();

#endif