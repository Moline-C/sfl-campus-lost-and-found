# Campus Lost-and-Found Tracker

**COP3530 Project 2 – Team SFL**

## Description

Finding a lost item on a large campus usually means retracing your steps and visiting every building you were in. This program lets a student search one collection of 100,000 lost-and-found reports instead. The student picks the item's category, name, color, last known location, and approximate date lost in a web page. Every report gets a match score, the reports are ranked with our own **merge sort** and **quicksort** implementations, and the top 10 matches are shown as claim tags that highlight which details matched. A second page compares the two sorting algorithms (runtime and number of comparisons) on the same data.

Match scoring: category = 4 points, item name = 3, color = 2, location = 2, found within 7 days on or after the loss date = 1 (12 points maximum). Ties are broken by the smaller report ID.

All data is synthetic (generated with a fixed random seed). No real student information is used.

## Team Members

- Moline Charles – matching and scoring system, quicksort
- Abigail Hepburn – dataset generation, CSV loading, merge sort
- Junia Celestin – web interface and terminal menu, performance measurements, test cases

## Tools Used

- C++17 (compiled with g++)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) (MIT license, header-only, included in `third_party/`) to serve the web page from the C++ program
- HTML, CSS and JavaScript for the browser interface (no external libraries)
- CSV files for report storage
- `std::chrono` for timing the sorts
- Git and GitHub for collaboration

## How to Compile and Run

From the root of the repository (needs g++ with C++17).

**Web interface (main way to use the program):**

```bash
# Windows (MinGW)
g++ -std=c++17 -O2 -D_WIN32_WINNT=0x0A00 -DUSE_TEAM_CODE -I. -Isrc -Ithird_party src/server.cpp load_csv.cpp merge_sort.cpp matching.cpp quick_sort.cpp -o lostfound_web -lws2_32

# macOS / Linux
g++ -std=c++17 -O2 -pthread -DUSE_TEAM_CODE -I. -Isrc -Ithird_party src/server.cpp load_csv.cpp merge_sort.cpp matching.cpp quick_sort.cpp -o lostfound_web

# Run it (from the repository root, so the web/ folder is found)
./lostfound_web reports.csv        # on Windows: .\lostfound_web reports.csv
```

The page opens in your browser at http://localhost:8080 (open it yourself if it does not). To use another port, add it at the end: `./lostfound_web reports.csv 8081`. The server only accepts connections from your own computer.

The page has three menu items:

1. **Find an item** – choose any of category, item, color, last place seen, and date lost, pick merge sort or quicksort, and see the top 10 matches with their scores, the sorting time, and the comparison count.
2. **Compare sorting** – runs merge sort and quicksort on identical copies of the same scored reports at several sizes (up to all 100,000), starting orders, and repeated scores, and shows the time and comparison count for each.
3. **Quit** – stops the program.

Each match also has a **Mark as collected** button. Use it once the owner has verified the item in person: the report is removed from the search right away, and its ID is saved in `resolved_reports.txt` so it stays hidden the next time the program starts. `reports.csv` itself is never changed. Delete `resolved_reports.txt` to bring every report back.

`reports.csv` holds 100,000 synthetic reports. To regenerate it: `make generate`.

**Terminal menu (same features, no browser):**

```bash
g++ -std=c++17 -O2 -DUSE_TEAM_CODE -I. -Isrc src/main.cpp load_csv.cpp merge_sort.cpp matching.cpp quick_sort.cpp -o lostfound
./lostfound reports.csv
```

## Running the Tests

All commands run from the repository root.

```bash
# 1. Sorting tests (sizes 0 to 100,000, repeated scores, different starting orders, same ranking)
g++ -std=c++17 -O2 -DUSE_TEAM_CODE -I. -Isrc tests/test_sorts.cpp load_csv.cpp merge_sort.cpp matching.cpp quick_sort.cpp -o test_sorts
./test_sorts

# 2. Feature tests (loading, dataset, scoring, ranking, top matches, timing, comparison, mark as collected)
g++ -std=c++17 -O2 -DUSE_TEAM_CODE -I. -Isrc tests/test_features.cpp load_csv.cpp merge_sort.cpp matching.cpp quick_sort.cpp -o test_features
./test_features reports.csv

# 3. Web interface tests: start the web server in one terminal, then in another:
python tests/test_web.py
```

`test_sorts` runs both sorts on dataset sizes from 0 up to 100,000, with random, already-ranked, reverse-ranked, and nearly-ranked starting orders (the ordered ones up to 5,000 reports), and with unique, repeated, and all-equal scores. Each case checks that the output is correctly ranked and that merge sort and quicksort give the same ranking. `test_features` and `test_web.py` check each feature of the program one by one and print PASS or FAIL for every check. Each program prints `Failed: 0` when everything passes. `test_web.py` removes one report through the Mark as collected feature; delete `resolved_reports.txt` and restart the server to bring it back.

---

