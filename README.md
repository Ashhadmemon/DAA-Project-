# CS2009 DAA Project - Part 2 and Part 3 (C++)

## The 3 programs (all use cout / cin and `using namespace std`)
| Program | What it does |
|---|---|
| `generator.cpp` | Part 2: creates the 20 input files (cp01-cp10, im01-im10) and `seeds.txt` |
| `closest_pair.cpp` | Divide and conquer closest pair of points (Kleinberg 5.4), O(n log n), own merge sort |
| `karatsuba.cpp` | Karatsuba multiplication, school multiplication for small numbers |

## How to compile and run  (keep all files in the SAME folder)
    g++ -O2 generator.cpp    -o generator
    g++ -O2 closest_pair.cpp -o closest_pair
    g++ -O2 karatsuba.cpp    -o karatsuba

    ./generator                 (creates the 20 files; takes a few seconds)
    ./closest_pair              (type  all  to run cp01..cp10, or a file name like cp10.txt)
    ./karatsuba                 (type  all  to run im01..im10, or a file name like im10.txt)

`-O2` makes the program much faster; it is needed for the 1,000,000 size inputs.
The programs also write `closest_pair_results.csv` and `karatsuba_results.csv` (Part 3 results).
Full products of long multiplications are saved as `imNN_product.txt`.

## Seeds
cpNN uses seed 2026100 + NN, imNN uses seed 2026200 + NN. They are listed in `seeds.txt`.
Running the generator again always gives exactly the same files.

## Checking the answers
- Closest pair: compared with trying every pair (n <= 5000) or with a simple sort-and-scan method (larger n).
- Karatsuba: remainder test with 3 different numbers, plus school multiplication for numbers up to 100,000 digits.
  All 10 products were also compared with Python's exact big-integer multiplication (all equal).

## Results (one run, times of the algorithm only)
| File | n | Time (ms) | Distance | Check |
|---|---|---|---|---|
| cp01 | 1,000 | 0.5 | 1147.935868 | PASS |
| cp02 | 10,000 | 6.2 | 61.212990 | PASS |
| cp03 | 100,000 | 74 | 1.329020 | PASS |
| cp04 | 1,000,000 | 950 | 0.714689 | PASS |
| cp05 | 1,000,000 | 939 | 0.030000 | PASS |
| cp06 | 1,000,000 | 775 | 0 | PASS |
| cp07 | 100,000 | 61 | 0.000002 | PASS |
| cp08 | 100,000 | 54 | 1000 | PASS |
| cp09 | 1,000,000 | 933 | 1.030776 | PASS |
| cp10 | 101 | 0.04 | 7.516025 | PASS |

| File | Digits | Time (ms) | Product digits | Check |
|---|---|---|---|---|
| im01 | 1,000 / 1,000 | 0.06 | 2,000 | PASS |
| im02 | 10,000 / 10,000 | 3.1 | 20,000 | PASS |
| im03 | 100,000 / 100,000 | 69 | 200,000 | PASS |
| im04 | 1,000,000 / 1,000,000 | 2006 | 2,000,000 | PASS |
| im05 | 1,000,000 / 10 | 0.9 | 1,000,010 | PASS |
| im06 | 100,000 / 100,000 | 58 | 200,000 | PASS |
| im07 | 99,999 / 99,999 | 55 | 199,998 | PASS |
| im08 | 1,000,000 / 1,000,000 | 37 | 1,999,999 | PASS |
| im09 | 1 / 1,000,000 | 0.004 | 1 | PASS |
| im10 | 101 / 101 | 0.001 | 202 | PASS |

## Submission note
Only files smaller than 1 MB are included here (cp01, cp02, cp10, im01, im02, im03, im06, im07, im10).
The other files are created again by `generator.cpp` using the seeds.
