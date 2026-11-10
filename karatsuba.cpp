#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
using namespace std;

typedef vector<long long> Big;           // a big number, 9 digits in every element
const long long BASE = 1000000000;       // 10^9
const int THRESHOLD = 32;                // use school multiplication up to this many blocks

// remove the zero blocks at the end (the front of the number). The number 0 is an empty vector.
void removeZeros(Big& a) {
    while (!a.empty() && a.back() == 0) a.pop_back();
}

// "1234567890123"  ->  { 567890123, 1234 }
Big stringToBig(string s) {
    Big a;
    int end = s.size();
    while (end > 0) {
        int start = end - 9;
        if (start < 0) start = 0;
        long long block = 0;
        for (int i = start; i < end; i++)
            block = block * 10 + (s[i] - '0');
        a.push_back(block);
        end = start;
    }
    removeZeros(a);
    return a;
}

// { 567890123, 1234 }  ->  "1234567890123"
string bigToString(Big a) {
    if (a.empty()) return "0";
    string s = to_string(a[a.size() - 1]);          // the first block is written without zeros in front
    for (int i = (int)a.size() - 2; i >= 0; i--) {
        string block = to_string(a[i]);
        s = s + string(9 - block.size(), '0') + block;   // other blocks must have exactly 9 digits
    }
    return s;
}

// a + b
Big addBig(const Big& a, const Big& b) {
    Big result;
    long long carry = 0;
    int n = max(a.size(), b.size());
    for (int i = 0; i < n; i++) {
        long long sum = carry;
        if (i < a.size()) sum = sum + a[i];
        if (i < b.size()) sum = sum + b[i];
        result.push_back(sum % BASE);
        carry = sum / BASE;
    }
    if (carry > 0) result.push_back(carry);
    return result;
}

// a - b   (a must be bigger or equal to b)
Big subtractBig(const Big& a, const Big& b) {
    Big result;
    long long borrow = 0;
    for (int i = 0; i < a.size(); i++) {
        long long diff = a[i] - borrow;
        if (i < b.size()) diff = diff - b[i];
        if (diff < 0) {
            diff = diff + BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.push_back(diff);
    }
    removeZeros(result);
    return result;
}

// a * B^k  (put k empty blocks in front)
Big shiftBig(const Big& a, int k) {
    if (a.empty()) return a;
    Big result(k, 0);
    for (int i = 0; i < a.size(); i++) result.push_back(a[i]);
    return result;
}

// ordinary (school) multiplication, O(n * m)
Big schoolMultiply(const Big& a, const Big& b) {
    if (a.empty() || b.empty()) return Big();
    Big result(a.size() + b.size(), 0);
    for (int i = 0; i < a.size(); i++) {
        long long carry = 0;
        for (int j = 0; j < b.size(); j++) {
            long long cur = result[i + j] + a[i] * b[j] + carry;
            result[i + j] = cur % BASE;
            carry = cur / BASE;
        }
        result[i + b.size()] = carry;
    }
    removeZeros(result);
    return result;
}

// KARATSUBA
Big karatsuba(const Big& x, const Big& y) {
    if (x.empty() || y.empty()) return Big();                       // anything * 0 = 0
    if (x.size() <= THRESHOLD || y.size() <= THRESHOLD)             // small numbers: school method
        return schoolMultiply(x, y);

    int n = max(x.size(), y.size());
    int m = (n + 1) / 2;                                            // size of the lower half

    // cut x into x0 (lower m blocks) and x1 (the rest), same for y
    Big x0, x1, y0, y1;
    for (int i = 0; i < x.size(); i++) {
        if (i < m) x0.push_back(x[i]);
        else       x1.push_back(x[i]);
    }
    for (int i = 0; i < y.size(); i++) {
        if (i < m) y0.push_back(y[i]);
        else       y1.push_back(y[i]);
    }
    removeZeros(x0);
    removeZeros(y0);

    Big z0 = karatsuba(x0, y0);
    Big z2 = karatsuba(x1, y1);
    Big z1 = karatsuba(addBig(x0, x1), addBig(y0, y1));
    z1 = subtractBig(z1, z0);
    z1 = subtractBig(z1, z2);

    // x * y = z2 * B^(2m) + z1 * B^m + z0
    Big result = addBig(z0, shiftBig(z1, m));
    result = addBig(result, shiftBig(z2, 2 * m));
    removeZeros(result);
    return result;
}

// ------------------------------------------------------------------ checking (NOT part of the algorithm)
// remainder of a long decimal number divided by p
long long remainderOf(string s, long long p) {
    long long r = 0;
    for (int i = 0; i < s.size(); i++)
        r = (r * 10 + (s[i] - '0')) % p;
    return r;
}

// Is product really a * b ?
// 1) remainders: (a mod p) * (b mod p) must have the same remainder as the product, for 3 different p
// 2) when the numbers are not too long (up to 100,000 digits) we also compare with school multiplication
bool checkProduct(string a, string b, string product, string& how) {
    long long primes[3] = {1000000007, 998244353, 999999937};
    for (int i = 0; i < 3; i++) {
        long long p = primes[i];
        if (remainderOf(a, p) * remainderOf(b, p) % p != remainderOf(product, p)) {
            how = "remainder test failed";
            return false;
        }
    }
    how = "remainder test";
    if (a.size() <= 100000 && b.size() <= 100000) {
        string school = bigToString(schoolMultiply(stringToBig(a), stringToBig(b)));
        if (school != product) {
            how = "school multiplication gives a different answer";
            return false;
        }
        how = "remainder test + school multiplication";
    }
    return true;
}

// ------------------------------------------------------------------ reading a file and running
bool runOneFile(string fileName, ofstream& csv) {
    ifstream in(fileName);
    if (!in) {
        cout << "Cannot open " << fileName << endl;
        return false;
    }
    string a, b;
    in >> a >> b;
    in.close();

    Big x = stringToBig(a);
    Big y = stringToBig(b);

    clock_t start = clock();
    Big z = karatsuba(x, y);
    clock_t finish = clock();
    double ms = 1000.0 * (finish - start) / CLOCKS_PER_SEC;

    string product = bigToString(z);
    string how;
    bool correct = checkProduct(a, b, product, how);

    cout << "File          : " << fileName << endl;
    cout << "Digits        : " << a.size() << " and " << b.size() << endl;
    cout << "Product digits: " << product.size() << endl;
    cout << "Time          : " << fixed << setprecision(3) << ms << " ms" << endl;
    if (product.size() <= 200) {
        cout << "Product       : " << product << endl;
    } else {
        string outName = fileName.substr(0, fileName.size() - 4) + "_product.txt";
        ofstream out(outName);
        out << product << "\n";
        out.close();
        cout << "Product       : " << product.substr(0, 20) << " ... " << product.substr(product.size() - 20) << endl;
        cout << "Full product saved in " << outName << endl;
    }
    cout << "Check         : " << (correct ? "PASS" : "FAIL") << "  (" << how << ")" << endl;
    cout << endl;

    csv << fileName << "," << a.size() << "," << b.size() << "," << fixed << setprecision(3) << ms << ","
        << product.size() << "," << product.substr(0, 20) << "," << (correct ? "PASS" : "FAIL") << "\n";
    return correct;
}

int main() {
    string name;
    cout << "Enter the input file name (for example im10.txt), or type all to run im01 .. im10: ";
    cin >> name;

    ofstream csv("karatsuba_results.csv");
    csv << "file,digits_x,digits_y,time_ms,product_digits,product_first_20_digits,check\n";

    if (name == "all") {
        int passed = 0;
        for (int k = 1; k <= 10; k++) {
            string f = "im";
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
