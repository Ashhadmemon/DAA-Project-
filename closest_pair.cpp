#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <algorithm>
using namespace std;

struct Point {
    double x, y;
    int id;            // number of the point in the file (0, 1, 2, ...)
};

struct Answer {
    double dist2;      // (distance)^2 of the closest pair
    int a, b;          // ids of the two points
};

vector<char> inQ;      // inQ[id] = 1 if the point is in the left half Q, 0 if in R

// squared distance between two points (we compare squares, so no sqrt is needed)
double dist2(Point p, Point q) {
    double dx = p.x - q.x;
    double dy = p.y - q.y;
    return dx * dx + dy * dy;
}

// ------------------------------------------------------------------ our own merge sort
// is point a before point b ?   (byX = true: sort by x, ties by y.   byX = false: sort by y, ties by x)
bool isBefore(Point a, Point b, bool byX) {
    if (byX) {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    } else {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    }
}

// sorts v[left] ... v[right-1]
void mergeSort(vector<Point>& v, vector<Point>& temp, int left, int right, bool byX) {
    if (right - left < 2) return;
    int mid = (left + right) / 2;
    mergeSort(v, temp, left, mid, byX);
    mergeSort(v, temp, mid, right, byX);

    int i = left, j = mid, k = left;
    while (i < mid && j < right) {
        if (isBefore(v[j], v[i], byX)) { temp[k] = v[j]; j++; }
        else                           { temp[k] = v[i]; i++; }
        k++;
    }
    while (i < mid)   { temp[k] = v[i]; i++; k++; }
    while (j < right) { temp[k] = v[j]; j++; k++; }
    for (k = left; k < right; k++) v[k] = temp[k];
}

// ------------------------------------------------------------------ the algorithm
// brute force for 2 or 3 points (base case)
Answer bruteForce(const vector<Point>& P) {
    Answer best;
    best.dist2 = 1e30;
    best.a = -1;
    best.b = -1;
    for (int i = 0; i < P.size(); i++)
        for (int j = i + 1; j < P.size(); j++) {
            double d = dist2(P[i], P[j]);
            if (d < best.dist2) {
                best.dist2 = d;
                best.a = P[i].id;
                best.b = P[j].id;
            }
        }
    return best;
}

// Px = points sorted by x,  Py = the same points sorted by y
Answer closestRec(const vector<Point>& Px, const vector<Point>& Py) {
    int n = Px.size();
    if (n <= 3) return bruteForce(Px);

    // step 2: split into Q (left half) and R (right half)
    int mid = n / 2;
    vector<Point> Qx, Rx, Qy, Ry;
    for (int i = 0; i < mid; i++) {
        Qx.push_back(Px[i]);
        inQ[Px[i].id] = 1;
    }
    for (int i = mid; i < n; i++) {
        Rx.push_back(Px[i]);
        inQ[Px[i].id] = 0;
    }
    for (int i = 0; i < n; i++) {          // Qy and Ry keep the y-order of Py
        if (inQ[Py[i].id] == 1) Qy.push_back(Py[i]);
        else                    Ry.push_back(Py[i]);
    }
    double xstar = Qx[mid - 1].x;          // the dividing line (largest x in Q)

    // step 3: solve both halves
    Answer q = closestRec(Qx, Qy);
    Answer r = closestRec(Rx, Ry);
    Answer best = r;
    if (q.dist2 <= r.dist2) best = q;

    // step 4: points close to the dividing line, in y-order
    vector<Point> S;
    for (int i = 0; i < n; i++) {
        double dx = Py[i].x - xstar;
        if (dx * dx < best.dist2) S.push_back(Py[i]);
    }
    for (int i = 0; i < S.size(); i++) {
        for (int j = i + 1; j < S.size() && j <= i + 15; j++) {
            double dy = S[j].y - S[i].y;
            if (dy * dy >= best.dist2) break;       // too far away in y, stop
            double d = dist2(S[i], S[j]);
            if (d < best.dist2) {
                best.dist2 = d;
                best.a = S[i].id;
                best.b = S[j].id;
            }
        }
    }
    return best;
}

// sorts the points and calls the recursive function
Answer closestPair(const vector<Point>& pts) {
    int n = pts.size();
    vector<Point> Px = pts, Py = pts, temp(n);
    mergeSort(Px, temp, 0, n, true);
    mergeSort(Py, temp, 0, n, false);
    inQ.assign(n, 0);
    return closestRec(Px, Py);
}

// ------------------------------------------------------------------ checking (NOT part of the algorithm)
bool lessX(Point a, Point b) {
    if (a.x != b.x) return a.x < b.x;
    return a.y < b.y;
}

// Slow but simple answer used only to CHECK our result.
// n <= 5000 : try every pair.   Larger n : sort with the built-in sort and compare each point
// with the following points while they are closer than the best distance so far.
double referenceDist2(vector<Point> P) {
    int n = P.size();
    double best = 1e30;
    if (n <= 5000) {
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                best = min(best, dist2(P[i], P[j]));
        return best;
    }
    // use the direction in which the points are most spread out (avoids a slow case for points on a line)
    double minX = P[0].x, maxX = P[0].x, minY = P[0].y, maxY = P[0].y;
    for (int i = 0; i < n; i++) {
        minX = min(minX, P[i].x); maxX = max(maxX, P[i].x);
        minY = min(minY, P[i].y); maxY = max(maxY, P[i].y);
    }
    if (maxY - minY > maxX - minX)
        for (int i = 0; i < n; i++) swap(P[i].x, P[i].y);
    sort(P.begin(), P.end(), lessX);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) {
            double dx = P[j].x - P[i].x;
            if (dx * dx >= best) break;
            best = min(best, dist2(P[i], P[j]));
        }
    return best;
}

// ------------------------------------------------------------------ reading a file and running
// returns false if the file could not be read
bool runOneFile(string fileName, ofstream& csv) {
    ifstream in(fileName);
    if (!in) {
        cout << "Cannot open " << fileName << endl;
        return false;
    }
    int n;
    in >> n;
    vector<Point> pts(n);
    for (int i = 0; i < n; i++) {
        in >> pts[i].x >> pts[i].y;
        pts[i].id = i;
    }
    in.close();

    clock_t start = clock();
    Answer ans = closestPair(pts);
    clock_t finish = clock();
    double ms = 1000.0 * (finish - start) / CLOCKS_PER_SEC;

    double distance = sqrt(ans.dist2);
    double refDistance = sqrt(referenceDist2(pts));
    bool correct = fabs(distance - refDistance) <= 1e-9 * max(1.0, refDistance);

    Point A = pts[ans.a], B = pts[ans.b];
    cout << fixed << setprecision(6);
    cout << "File          : " << fileName << "   (n = " << n << ")" << endl;
    cout << "Closest pair  : (" << A.x << ", " << A.y << ")  and  (" << B.x << ", " << B.y << ")" << endl;
    cout << "Distance      : " << distance << endl;
    cout << "Time          : " << setprecision(3) << ms << " ms" << endl;
    cout << "Check         : " << (correct ? "PASS" : "FAIL") << "  (reference distance " << setprecision(6) << refDistance << ")" << endl;
    cout << endl;

    csv << fileName << "," << n << "," << setprecision(3) << ms << "," << setprecision(9) << distance << ","
        << setprecision(6) << A.x << "," << A.y << "," << B.x << "," << B.y << "," << (correct ? "PASS" : "FAIL") << "\n";
    return correct;
}

int main() {
    string name;
    cout << "Enter the input file name (for example cp10.txt), or type all to run cp01 .. cp10: ";
    cin >> name;

    ofstream csv("closest_pair_results.csv");
    csv << "file,n,time_ms,distance,x1,y1,x2,y2,check\n";

    if (name == "all") {
        int passed = 0;
        for (int k = 1; k <= 10; k++) {
            string f = "cp";
            if (k < 10) f = f + "0";
            f = f + to_string(k) + ".txt";
            if (runOneFile(f, csv)) passed++;
        }
        cout << passed << " of 10 files PASSED" << endl;
    } else {
        runOneFile(name, csv);
    }
    csv.close();
    return 0;
}
