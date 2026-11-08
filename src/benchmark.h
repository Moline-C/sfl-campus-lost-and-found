// benchmark.h  (Junia)
//
// Times merge sort and quicksort with std::chrono and reports how many
// comparisons each one makes. Both algorithms always get an identical COPY of
// the same scored reports, so the comparison is fair.
//
// The comparison counts come from the sorts themselves: runMergeSort and
// runQuickSort (see backend.h) return how many comparisons the algorithm made.
//
// Used by the terminal menu (main.cpp) and the web server (server.cpp).

#pragma once

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "backend.h"

struct SortRun {
    std::string name;
    double milliseconds = 0.0;
    unsigned long long comparisons = 0;
};

// sortFn(vector&) sorts the vector and returns its comparison count.
// `input` is never changed; the sorted copy is returned in `output`.
template <class SortFn>
SortRun timeSort(const std::string& name, const std::vector<ScoredReport>& input,
                 std::vector<ScoredReport>& output, SortFn sortFn) {
    output = input;  // the copy happens BEFORE the clock starts

    auto start = std::chrono::steady_clock::now();
    unsigned long long count = sortFn(output);
    auto stop = std::chrono::steady_clock::now();

    SortRun run;
    run.name = name;
    run.milliseconds = std::chrono::duration<double, std::milli>(stop - start).count();
    run.comparisons = count;
    return run;
}

inline SortRun timeMergeSort(const std::vector<ScoredReport>& in, std::vector<ScoredReport>& out) {
    return timeSort("Merge sort", in, out,
                    [](std::vector<ScoredReport>& v) { return runMergeSort(v); });
}

inline SortRun timeQuickSort(const std::vector<ScoredReport>& in, std::vector<ScoredReport>& out) {
    return timeSort("Quicksort", in, out,
                    [](std::vector<ScoredReport>& v) { return runQuickSort(v); });
}

// True when both vectors list the same reports in the same order.
inline bool sameRanking(const std::vector<ScoredReport>& a, const std::vector<ScoredReport>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].reportID != b[i].reportID || a[i].score != b[i].score) return false;
    }
    return true;
}

// ---------- one comparison of both sorts on the same input ----------
struct CompareRow {
    std::string label;
    size_t size = 0;
    SortRun merge;
    SortRun quick;
    bool same = false;  // did both sorts produce the same ranking?
};

inline CompareRow runCompare(const std::string& label, const std::vector<ScoredReport>& input) {
    std::vector<ScoredReport> mergeOut, quickOut;
    CompareRow row;
    row.label = label;
    row.size = input.size();
    row.merge = timeMergeSort(input, mergeOut);
    row.quick = timeQuickSort(input, quickOut);
    row.same = sameRanking(mergeOut, quickOut);
    return row;
}

inline void printCompareRow(const CompareRow& r) {
    std::cout << std::left << std::setw(30) << r.label << std::right << std::setw(9) << r.size
              << " | merge " << std::setw(9) << std::fixed << std::setprecision(2)
              << r.merge.milliseconds << " ms " << std::setw(11) << r.merge.comparisons << " cmp"
              << " | quick " << std::setw(9) << r.quick.milliseconds << " ms " << std::setw(11)
              << r.quick.comparisons << " cmp"
              << " | " << (r.same ? "same ranking" : "RANKING MISMATCH") << "\n";
}

// Size used for the "already ranked", "reverse ranked" and "repeated scores" cases.
// A quicksort that always picks the last report as its pivot needs about n*n/2
// comparisons on ordered data, so 100,000 reports could take minutes. 5,000
// shows the effect and finishes in seconds. Raise it if quicksort picks a
// better pivot.
constexpr size_t kOrderedCaseSize = 5000;

// The fixed sample search used by every comparison, so results are repeatable.
inline Query sampleQuery() {
    Query q;
    q.category = "electronics";
    q.itemName = "phone";
    q.color = "black";
    q.location = "library west";
    q.dateLost = 100;
    return q;
}

// Runs the standard set of comparisons and calls onRow(row) after each one:
//   - different dataset sizes (starting order = order in the data file)
//   - already ranked and reverse ranked starting orders
//   - many repeated scores
// Returns true if both sorts agreed on the ranking every time.
template <class RowFn>
bool runStandardComparison(const std::vector<Report>& reports, RowFn onRow) {
    std::vector<ScoredReport> all = scoreReports(reports, sampleQuery());
    bool allSame = true;
    auto emit = [&](const std::string& label, const std::vector<ScoredReport>& input) {
        CompareRow row = runCompare(label, input);
        allSame = allSame && row.same;
        onRow(row);
    };

    const size_t sizes[] = {1000, 10000, 50000, 100000};
    for (size_t n : sizes) {
        if (n > all.size()) continue;
        std::vector<ScoredReport> part(all.begin(), all.begin() + static_cast<long>(n));
        emit("File order, size " + std::to_string(n), part);
    }
    if (all.size() < 100000 && !all.empty()) emit("File order, all reports", all);

    std::vector<ScoredReport> firstReports(
        all.begin(), all.begin() + static_cast<long>(std::min(all.size(), kOrderedCaseSize)));
    std::vector<ScoredReport> rankedIn = firstReports, reverseIn;
    std::sort(rankedIn.begin(), rankedIn.end(), ranksBefore);
    reverseIn.assign(rankedIn.rbegin(), rankedIn.rend());
    emit("Already ranked (best first)", rankedIn);
    emit("Reverse ranked (worst first)", reverseIn);

    std::vector<ScoredReport> repeated = firstReports;
    for (size_t i = 0; i < repeated.size(); i++) repeated[i].score = static_cast<int>(i % 3);
    emit("Only 3 distinct scores", repeated);

    return allSame;
}

// Terminal version of the comparison (used by main.cpp).
inline bool compareOnInput(const std::string& label, const std::vector<ScoredReport>& input) {
    CompareRow r = runCompare(label, input);
    printCompareRow(r);
    return r.same;
}
