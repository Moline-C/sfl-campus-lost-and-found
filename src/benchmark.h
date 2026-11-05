// benchmark.h  (Junia)
//
// Times merge sort and quicksort with std::chrono and reports how many
// comparisons each one makes. Both algorithms always get an identical COPY of
// the same scored reports, so the comparison is fair.
//
// The comparison counts come from the sorts themselves: runMergeSort and
// runQuickSort (see backend.h) return how many comparisons the algorithm made.

#pragma once

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

// Runs both sorts on the same input and prints one row per comparison.
// Returns true if both produced the same ranking.
inline bool compareOnInput(const std::string& label, const std::vector<ScoredReport>& input) {
    std::vector<ScoredReport> mergeOut, quickOut;
    SortRun m = timeMergeSort(input, mergeOut);
    SortRun q = timeQuickSort(input, quickOut);
    bool same = sameRanking(mergeOut, quickOut);

    std::cout << std::left << std::setw(30) << label << std::right << std::setw(9) << input.size()
              << " | merge " << std::setw(9) << std::fixed << std::setprecision(2) << m.milliseconds
              << " ms " << std::setw(11) << m.comparisons << " cmp"
              << " | quick " << std::setw(9) << q.milliseconds << " ms " << std::setw(11)
              << q.comparisons << " cmp"
              << " | " << (same ? "same ranking" : "RANKING MISMATCH") << "\n";
    return same;
}
