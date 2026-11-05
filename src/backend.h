   // backend.h
   // Connects the menu, benchmark and tests to the team's code
   // (Report, loadReports, scoring, merge sort, quicksort).
   // Compile with -DUSE_TEAM_CODE to use the real implementations;
   // otherwise the temporary placeholder_backend.h is used.

#pragma once

#include <string>
#include <vector>

#ifdef USE_TEAM_CODE

#include "report.h"      // Abigail: Report, LostItem
#include "load_csv.h"    // Abigail: vector<Report> loadReports(const string&)  (throws)
#include "merge_sort.h"  // Abigail: void mergeSort(vector<Report>&, int& comparisons)
#include "matching.h"    // Moline:  int calculateScore(LostItem, Report)
#include "quick_sort.h"  // Moline:  void quickSort(vector<Report>&, int& comparisons)
                         //          (file name and signature still to be confirmed with Moline)

using ScoredReport = Report;
using Query = LostItem;

// Same rule both sorts must follow: highest score first, tie smaller reportID first.
inline bool ranksBefore(const ScoredReport& a, const ScoredReport& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.reportID < b.reportID;
}

inline std::vector<ScoredReport> scoreReports(const std::vector<Report>& reports, const Query& q) {
    LostItem lost = q;
    // calculateScore always checks the date, so when the user skips the date use a
    // value that can never match (found dates are 1-365).
    if (lost.dateLost <= 0) lost.dateLost = -1000;
    std::vector<ScoredReport> out = reports;
    for (ScoredReport& r : out) r.score = calculateScore(lost, r);
    return out;
}

inline unsigned long long runMergeSort(std::vector<ScoredReport>& v) {
    int comparisons = 0;
    mergeSort(v, comparisons);
    return static_cast<unsigned long long>(comparisons);
}

inline unsigned long long runQuickSort(std::vector<ScoredReport>& v) {
    int comparisons = 0;
    quickSort(v, comparisons);
    return static_cast<unsigned long long>(comparisons);
}

#else

#include "placeholder_backend.h"

#endif
