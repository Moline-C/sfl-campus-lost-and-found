// test_features.cpp  (Junia)
//
// One group of tests for each feature in the project proposal:
//   F1  Load reports from a CSV file
//   F2  Dataset: 100,000 reports with the agreed fields
//   F3  Matching score: category 4, item 3, color 2, location 2, date 1
//   F4  Ranking: highest score first, ties broken by smaller report ID
//   F5  Merge sort and quicksort give the same ranking on the real data
//   F6  Top matches: the best report is ranked first, 10 are shown
//   F7  Measurements: runtime (std::chrono) and number of comparisons
//   F8  Comparison of the two sorts at different sizes, orders and repeated scores
//   F9  Searches that match few details finish in a reasonable time
//   F10 Mark as collected: a verified report is removed from the database
//
// Build (from the repository root, with team's code):
//   g++ -std=c++17 -O2 -DUSE_TEAM_CODE -I. -Isrc tests/test_features.cpp load_csv.cpp merge_sort.cpp matching.cpp quick_sort.cpp -o test_features
// Run:
//   ./test_features [path/to/reports.csv]     (default: reports.csv)
// Exit code 0 means every test passed.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "backend.h"
#include "benchmark.h"
#include "resolved.h"

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (ok) g_pass++;
    else g_fail++;
}

static void feature(const std::string& title) { std::cout << "\n" << title << "\n"; }

static Report makeReport(int id, const std::string& category, const std::string& item,
                         const std::string& color, const std::string& location, int day) {
    Report r;
    r.reportID = id;
    r.category = category;
    r.itemName = item;
    r.color = color;
    r.location = location;
    r.dateFound = day;
    return r;
}

// Score of one report for one search, through the same path the program uses.
static int scoreOf(const Report& r, const Query& q) {
    return scoreReports(std::vector<Report>{r}, q)[0].score;
}

static Query makeQuery(const std::string& category, const std::string& item, const std::string& color,
                       const std::string& location, int day) {
    Query q;
    q.category = category;
    q.itemName = item;
    q.color = color;
    q.location = location;
    q.dateLost = day;
    return q;
}

static bool isRanked(const std::vector<ScoredReport>& v) {
    for (size_t i = 1; i < v.size(); i++) {
        if (ranksBefore(v[i], v[i - 1])) return false;
    }
    return true;
}

static void writeFile(const std::string& path, const std::string& text) {
    std::ofstream out(path);
    out << text;
}

template <class Fn>
static bool throws(Fn fn) {
    try {
        fn();
    } catch (...) {
        return true;
    }
    return false;
}

int main(int argc, char** argv) {
    std::string path = (argc > 1) ? argv[1] : "reports.csv";

    // F1
    feature("F1  Load reports from a CSV file");
    std::vector<Report> reports;
    bool loaded = false;
    try {
        reports = loadReports(path);
        loaded = true;
    } catch (const std::exception& e) {
#ifndef USE_TEAM_CODE
        reports = generateReports(100000, 42);  // placeholder build: no real file needed
        loaded = true;
        std::cout << "  (placeholder build: using generated sample data)\n";
#else
        std::cout << "  FAIL  could not load " << path << ": " << e.what() << "\n";
        g_fail++;
#endif
    }
    if (!loaded) {
        std::cout << "\nStopping: the remaining tests need the data file. Run from the repository root.\n";
        std::cout << "Passed: " << g_pass << "   Failed: " << g_fail << "\n";
        return 1;
    }
    check(reports.size() == 100000, "loads all 100,000 reports (got " + std::to_string(reports.size()) + ")");
    check(throws([] { loadReports("this_file_does_not_exist.csv"); }), "a missing file is reported as an error");
#ifdef USE_TEAM_CODE
    writeFile("feature_test_tmp.csv", "wrong,header\n1,bags,wallet,red,the hub,10\n");
    check(throws([] { loadReports("feature_test_tmp.csv"); }), "a wrong CSV header is rejected");
    writeFile("feature_test_tmp.csv", "reportID,category,itemName,color,location,dateFound\n1,bags,wallet,red,the hub\n");
    check(throws([] { loadReports("feature_test_tmp.csv"); }), "a row with a missing field is rejected");
    writeFile("feature_test_tmp.csv",
              "reportID,category,itemName,color,location,dateFound\n1,bags,wallet,red,the hub,10\n"
              "1,bags,purse,blue,the hub,11\n");
    check(throws([] { loadReports("feature_test_tmp.csv"); }), "a duplicate report ID is rejected");
    std::remove("feature_test_tmp.csv");
#endif

    // F2
    feature("F2  Dataset: reports with the agreed fields");
    {
        std::unordered_set<int> ids;
        std::set<std::string> categories, colors, locations;
        bool fieldsFilled = true, daysOk = true;
        for (const Report& r : reports) {
            ids.insert(r.reportID);
            categories.insert(r.category);
            colors.insert(r.color);
            locations.insert(r.location);
            if (r.category.empty() || r.itemName.empty() || r.color.empty() || r.location.empty()) fieldsFilled = false;
            if (r.dateFound < 1 || r.dateFound > 365) daysOk = false;
        }
        check(ids.size() == reports.size(), "every report ID is unique");
        check(fieldsFilled, "category, item name, color and location are filled in for every report");
        check(daysOk, "dateFound is a day number from 1 to 365 for every report");
        check(categories.size() >= 5 && colors.size() >= 5 && locations.size() >= 5,
              "there is variety to search on (" + std::to_string(categories.size()) + " categories, " +
                  std::to_string(colors.size()) + " colors, " + std::to_string(locations.size()) + " locations)");
    }

    // F3
    feature("F3  Matching score");
    {
        Report r = makeReport(1, "bags", "backpack", "red", "the hub", 100);
        check(scoreOf(r, makeQuery("bags", "", "", "", 0)) == 4, "matching category = 4 points");
        check(scoreOf(r, makeQuery("", "backpack", "", "", 0)) == 3, "matching item name = 3 points");
        check(scoreOf(r, makeQuery("", "", "red", "", 0)) == 2, "matching color = 2 points");
        check(scoreOf(r, makeQuery("", "", "", "the hub", 0)) == 2, "matching location = 2 points");
        check(scoreOf(r, makeQuery("", "", "", "", 100)) == 1, "found on the day it was lost = 1 point");
        check(scoreOf(r, makeQuery("", "", "", "", 93)) == 1, "found 7 days after the loss date = 1 point");
        check(scoreOf(r, makeQuery("", "", "", "", 92)) == 0, "found 8 days after the loss date = 0 points");
        check(scoreOf(r, makeQuery("", "", "", "", 101)) == 0, "found before the loss date = 0 points");
        check(scoreOf(r, makeQuery("bags", "backpack", "red", "the hub", 100)) == 12, "everything matching = 12 points");
        check(scoreOf(r, makeQuery("clothing", "hat", "blue", "newell hall", 0)) == 0, "nothing matching = 0 points");
        check(scoreOf(r, makeQuery("", "", "", "", 0)) == 0, "skipping the date never earns the date point");
    }

    // F4
    feature("F4  Ranking: highest score first, ties by smaller report ID");
    {
        std::vector<ScoredReport> base;
        const int ids[] = {10, 7, 3, 99, 8};
        const int scores[] = {3, 5, 5, 1, 5};
        for (int i = 0; i < 5; i++) {
            ScoredReport s = makeReport(ids[i], "c", "i", "k", "l", 1);
            s.score = scores[i];
            base.push_back(s);
        }
        const int expected[] = {3, 7, 8, 10, 99};
        std::vector<ScoredReport> m = base, q = base;
        runMergeSort(m);
        runQuickSort(q);
        bool mOk = true, qOk = true;
        for (int i = 0; i < 5; i++) {
            mOk = mOk && m[i].reportID == expected[i];
            qOk = qOk && q[i].reportID == expected[i];
        }
        check(mOk, "merge sort orders [3, 5, 5, 1, 5] as IDs 3, 7, 8, 10, 99");
        check(qOk, "quicksort orders [3, 5, 5, 1, 5] as IDs 3, 7, 8, 10, 99");
    }

    // F5
    feature("F5  Merge sort and quicksort agree on the real data");
    std::vector<ScoredReport> scoredSample = scoreReports(reports, sampleQuery());
    {
        const size_t sizes[] = {1000, 10000, 50000, 100000};
        for (size_t n : sizes) {
            std::vector<ScoredReport> part(scoredSample.begin(), scoredSample.begin() + static_cast<long>(n));
            std::vector<ScoredReport> m, q;
            timeMergeSort(part, m);
            timeQuickSort(part, q);
            check(isRanked(m) && isRanked(q) && sameRanking(m, q),
                  "both sorts give the same, correctly ranked result for " + std::to_string(n) + " reports");
        }
    }

    // F6
    feature("F6  Top matches");
    {
        // Lowest ID of all, so it also wins any tie at score 12 (ties go to the smaller ID).
        int plantedId = reports[0].reportID;
        for (const Report& r : reports) plantedId = std::min(plantedId, r.reportID);
        plantedId--;
        std::vector<Report> withPlanted = reports;
        Query q = sampleQuery();
        withPlanted.push_back(makeReport(plantedId, q.category, q.itemName, q.color, q.location, q.dateLost));
        std::vector<ScoredReport> scored = scoreReports(withPlanted, q), m, qs;
        timeMergeSort(scored, m);
        timeQuickSort(scored, qs);
        check(m[0].reportID == plantedId && m[0].score == 12, "a report matching every detail (lowest ID) is ranked first by merge sort (score 12)");
        check(qs[0].reportID == plantedId && qs[0].score == 12, "a report matching every detail (lowest ID) is ranked first by quicksort (score 12)");
        bool nonIncreasing = true;
        for (size_t i = 1; i < 10; i++) nonIncreasing = nonIncreasing && m[i].score <= m[i - 1].score;
        check(m.size() >= 10 && nonIncreasing, "the top 10 are in non-increasing score order");
        std::vector<ScoredReport> again;
        timeMergeSort(scored, again);
        check(sameRanking(m, again), "the same search gives the same ranking every time");
    }

    // F7
    feature("F7  Measurements: runtime and comparisons");
    {
        std::vector<ScoredReport> snapshot = scoredSample, m, q;
        SortRun mr = timeMergeSort(scoredSample, m);
        SortRun qr = timeQuickSort(scoredSample, q);
        const double n = static_cast<double>(scoredSample.size());
        check(sameRanking(snapshot, scoredSample), "the input is not changed, so both sorts get an identical copy");
        check(std::isfinite(mr.milliseconds) && mr.milliseconds >= 0 && std::isfinite(qr.milliseconds) && qr.milliseconds >= 0,
              "runtime is measured for both sorts (merge " + std::to_string(mr.milliseconds) + " ms, quick " +
                  std::to_string(qr.milliseconds) + " ms)");
        check(mr.comparisons >= n - 1 && qr.comparisons >= n - 1, "both sorts count at least n-1 comparisons");
        check(static_cast<double>(mr.comparisons) <= n * std::ceil(std::log2(n)),
              "merge sort counts at most n*ceil(log2 n) comparisons (" + std::to_string(mr.comparisons) + ")");
    }

    // F8
    feature("F8  Compare the sorts at different sizes, orders and repeated scores");
    {
        std::vector<std::string> labels;
        std::vector<size_t> sizes;
        bool allSame = runStandardComparison(reports, [&](const CompareRow& row) {
            labels.push_back(row.label);
            sizes.push_back(row.size);
        });
        auto has = [&](const std::string& part) {
            for (const std::string& l : labels)
                if (l.find(part) != std::string::npos) return true;
            return false;
        };
        check(allSame, "both sorts gave the same ranking in every comparison");
        check(std::find(sizes.begin(), sizes.end(), 100000u) != sizes.end(), "the full 100,000 reports are compared");
        check(has("size 1000") && has("size 10000") && has("size 50000"), "smaller dataset sizes are compared too");
        check(has("Already ranked") && has("Reverse ranked"), "different starting orders are compared");
        check(has("3 distinct scores"), "repeated scores are compared");
    }

    // F9
    feature("F9  Searches with few details finish in a reasonable time (under 3 seconds each)");
    {
        struct Case { const char* name; Query q; };
        Case cases[] = {{"category only", makeQuery("electronics", "", "", "", 0)},
                        {"color only", makeQuery("", "", "black", "", 0)},
                        {"location only", makeQuery("", "", "", "library west", 0)}};
        for (const Case& c : cases) {
            std::vector<ScoredReport> scored = scoreReports(reports, c.q), m, q;
            SortRun mr = timeMergeSort(scored, m);
            SortRun qr = timeQuickSort(scored, q);
            check(mr.milliseconds < 3000, std::string("merge sort, ") + c.name + ": " + std::to_string(mr.milliseconds) + " ms");
            check(qr.milliseconds < 3000, std::string("quicksort, ") + c.name + ": " + std::to_string(qr.milliseconds) + " ms");
            check(sameRanking(m, q), std::string("same ranking, ") + c.name);
        }
    }

    // F10
    feature("F10 Mark as collected removes the report");
    {
        std::vector<Report> db = reports;
        int target = db[500].reportID;
        Query q = makeQuery(db[500].category, db[500].itemName, db[500].color, db[500].location, db[500].dateFound);
        std::vector<ScoredReport> before, after;
        timeMergeSort(scoreReports(db, q), before);
        bool wasFirst = !before.empty() && before[0].score == 12;
        check(removeReportById(db, target), "removing an existing report succeeds");
        check(db.size() == reports.size() - 1, "exactly one report is removed");
        check(!removeReportById(db, target), "removing the same report again fails");
        check(!removeReportById(db, -5), "removing an unknown ID fails");
        timeMergeSort(scoreReports(db, q), after);
        bool stillThere = false;
        for (const ScoredReport& s : after)
            if (s.reportID == target) stillThere = true;
        check(wasFirst && !stillThere, "the removed report no longer appears in search results");
        std::vector<Report> db2 = reports;
        check(removeResolved(db2, {reports[1].reportID, reports[2].reportID, 424242}) == 2 && db2.size() == reports.size() - 2,
              "reports saved as collected are skipped at the next start");
        std::remove("feature_test_resolved.txt");
        check(appendResolvedId("feature_test_resolved.txt", 17) && appendResolvedId("feature_test_resolved.txt", 23) &&
                  loadResolvedIds("feature_test_resolved.txt") == std::vector<int>({17, 23}),
              "collected report IDs are saved to a file and read back");
        std::remove("feature_test_resolved.txt");
    }

    std::cout << "\nPassed: " << g_pass << "   Failed: " << g_fail << "\n";
    return g_fail == 0 ? 0 : 1;
}
