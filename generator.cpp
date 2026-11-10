#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <random>
using namespace std;

mt19937 gen;   // random number generator. gen() gives a random number. We set its seed before every file.

struct Point {
    double x, y;
};

// random whole number from 0 to n-1
long long randBelow(long long n) {
    return gen() % n;
}

// random real number from 0 to 1000000 (3 digits after the decimal point)
double randCoord() {
    return randBelow(1000000000) / 1000.0;
}

// random real number from low to high (3 digits after the decimal point)
double randBetween(double low, double high) {
    return low + randBelow(1000000) / 1000000.0 * (high - low);
}

// approximately a "bell curve" number between -6 and 6 (sum of 12 random numbers minus 6)
double bell() {
    double sum = 0;
    for (int i = 0; i < 12; i++)
        sum = sum + randBelow(1000000) / 1000000.0;
    return sum - 6;
}

// mix the points in random order (Fisher-Yates shuffle)
void shufflePoints(vector<Point>& v) {
    for (int i = v.size() - 1; i >= 1; i--) {
        int j = randBelow(i + 1);
        swap(v[i], v[j]);
    }
}

// random number with exactly 'len' digits, first digit is not 0
string randomNumber(int len) {
    string s = "";
    s = s + char('1' + randBelow(9));
    for (int i = 1; i < len; i++)
        s = s + char('0' + randBelow(10));
    return s;
}

// write the points to a file: first line n, then one "x y" per line
void writePoints(string fileName, vector<Point>& v, bool wholeNumbers, int decimals) {
    ofstream out(fileName);
    out << v.size() << "\n";
    if (wholeNumbers) {
        for (int i = 0; i < v.size(); i++)
            out << (long long)v[i].x << " " << (long long)v[i].y << "\n";
    } else {
        out << fixed << setprecision(decimals);
        for (int i = 0; i < v.size(); i++)
            out << v[i].x << " " << v[i].y << "\n";
    }
    out.close();
}

// write two numbers, one per line
void writeNumbers(string fileName, string a, string b) {
    ofstream out(fileName);
    out << a << "\n" << b << "\n";
    out.close();
}

// ---------------------------------------------------------------------------
void makeClosestPairFile(int k, ofstream& seedsFile) {
    string name = "cp";
    if (k < 10) name = name + "0";
    name = name + to_string(k);

    int seed = 2026100 + k;
    gen.seed(seed);

    vector<Point> pts;
    bool wholeNumbers = false;
    int decimals = 3;          // digits after the decimal point written in the file
    string description = "";

    if (k >= 1 && k <= 4) {
        // cp01..cp04 : 10^3, 10^4, 10^5, 10^6 uniform random points in [0, 10^6]
        int n = 1000;
        for (int i = 1; i < k; i++) n = n * 10;
        for (int i = 0; i < n; i++) {
            Point p;
            p.x = randCoord();
            p.y = randCoord();
            pts.push_back(p);
        }
        description = "uniform random real coordinates in [0, 10^6]";
    }
    else if (k == 5) {
        // 1,000,000 points around 10 random centers
        double cx[10], cy[10];
        for (int i = 0; i < 10; i++) {
            cx[i] = 100000 + randBelow(800000000) / 1000.0;
            cy[i] = 100000 + randBelow(800000000) / 1000.0;
        }
        for (int i = 0; i < 1000000; i++) {
            int c = randBelow(10);
            Point p;
            p.x = cx[c] + 10000 * bell();
            p.y = cy[c] + 10000 * bell();
            pts.push_back(p);
        }
        description = "clustered around 10 random centers (spread about 10000)";
    }
    else if (k == 6) {
        // integer coordinates in a small range -> many duplicate points
        for (int i = 0; i < 1000000; i++) {
            Point p;
            p.x = randBelow(1000);
            p.y = randBelow(1000);
            pts.push_back(p);
        }
        wholeNumbers = true;
        description = "integer coordinates in [0, 999], many duplicates (closest distance 0)";
    }
    else if (k == 7) {
        // all points on one vertical line x = 500000
        for (int i = 0; i < 100000; i++) {
            Point p;
            p.x = 500000;
            p.y = (randBelow(1000000) * 1000000 + randBelow(1000000)) / 1000000.0;   // 6 decimals, so no equal points
            pts.push_back(p);
        }
        decimals = 6;
        description = "all points on the vertical line x = 500000";
    }
    else if (k == 8) {
        // equally spaced grid 400 x 250 = 100000 points, distance 1000 between neighbours
        for (int i = 0; i < 400; i++)
            for (int j = 0; j < 250; j++) {
                Point p;
                p.x = i * 1000;
                p.y = j * 1000;
                pts.push_back(p);
            }
        shufflePoints(pts);
        description = "400 x 250 equally spaced grid, spacing 1000 (many tied closest pairs)";
    }
    else if (k == 9) {
        // A 1000 x 1000 grid with cells of size 1000. Each point is moved randomly by at most 200,
        // so two normal points are always at least 600 apart.
        // Two cells next to the middle line x = 500000 are replaced by a special pair
        // (distance about 1.03) with one point on each side of the middle line.
        for (int i = 0; i < 1000; i++)
            for (int j = 0; j < 1000; j++) {
                if (j == 500 && (i == 499 || i == 500)) continue;   // skip the two cells
                Point p;
                p.x = (i + 0.5) * 1000 + randBetween(-200, 200);
                p.y = (j + 0.5) * 1000 + randBetween(-200, 200);
                pts.push_back(p);
            }
        Point left, right;
        left.x = 499999.5;  left.y = 500500.0;
        right.x = 500000.5; right.y = 500500.25;
        pts.push_back(left);
        pts.push_back(right);
        shufflePoints(pts);
        description = "closest pair (distance about 1.03) lies across the middle dividing line x = 500000";
    }
    else if (k == 10) {
        for (int i = 0; i < 101; i++) {
            Point p;
            p.x = randBelow(1000000) / 1000.0;
            p.y = randBelow(1000000) / 1000.0;
            pts.push_back(p);
        }
        description = "101 random points in [0, 1000], small file for step by step";
    }

    writePoints(name + ".txt", pts, wholeNumbers, decimals);
    seedsFile << name << ".txt  seed=" << seed << "  n=" << pts.size() << "  " << description << "\n";
    cout << "created " << name << ".txt  (" << pts.size() << " points)" << endl;
}

// ---------------------------------------------------------------------------
void makeMultiplicationFile(int k, ofstream& seedsFile) {
    string name = "im";
    if (k < 10) name = name + "0";
    name = name + to_string(k);

    int seed = 2026200 + k;
    gen.seed(seed);

    string a, b, description;

    if (k >= 1 && k <= 4) {
        // 10^3, 10^4, 10^5, 10^6 digits
        int len = 1000;
        for (int i = 1; i < k; i++) len = len * 10;
        a = randomNumber(len);
        b = randomNumber(len);
        description = "two random numbers, same number of digits, no leading zero";
    }
    else if (k == 5) {
        a = randomNumber(1000000);
        b = randomNumber(10);
        description = "1,000,000 digits and 10 digits (very different lengths)";
    }
    else if (k == 6) {
        a = string(100000, '9');
        b = randomNumber(100000);
        description = "100,000 digits, first number is all 9s";
    }
    else if (k == 7) {
        // 99,999 digits (odd length), about 80% of the digits are 0
        for (int t = 0; t < 2; t++) {
            string s = "";
            s = s + char('1' + randBelow(9));
            for (int i = 1; i < 99999; i++) {
                if (randBelow(10) < 8) s = s + '0';
                else s = s + char('1' + randBelow(9));
            }
            if (t == 0) a = s; else b = s;
        }
        description = "99,999 digits (odd length), about 80% zeros";
    }
    else if (k == 8) {
        a = "1" + string(999999, '0');   // 10^999999
        b = a;
        description = "both numbers are powers of 10 (10^999999)";
    }
    else if (k == 9) {
        a = "0";
        b = randomNumber(1000000);
        description = "one number is 0 (1 digit), the other has 1,000,000 digits";
    }
    else if (k == 10) {
        a = randomNumber(101);
        b = randomNumber(101);
        description = "101 digits each, small file for step by step";
    }

    writeNumbers(name + ".txt", a, b);
    seedsFile << name << ".txt  seed=" << seed << "  digits=" << a.size() << " and " << b.size() << "  " << description << "\n";
    cout << "created " << name << ".txt  (" << a.size() << " and " << b.size() << " digits)" << endl;
}

int main() {
    ofstream seedsFile("seeds.txt");
    for (int k = 1; k <= 10; k++) makeClosestPairFile(k, seedsFile);
    for (int k = 1; k <= 10; k++) makeMultiplicationFile(k, seedsFile);
    seedsFile.close();
    cout << "Done. Seeds are saved in seeds.txt" << endl;
    return 0;
}
