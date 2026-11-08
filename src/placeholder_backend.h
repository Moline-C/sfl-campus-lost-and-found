// placeholder_backend.h
//
// TEMPORARY stand-in for your teammates' code so the menu, benchmark and tests
// can be built before everything is merged. It copies the SAME shapes as the
// real code on the team branches:
//   Report (with a score field), LostItem, loadReports(file) that throws,
//   calculateScore-style scoring, and sorts that count their own comparisons.
// NOT part of the final project: don't submit it. Compile WITHOUT
// -DUSE_TEAM_CODE to use it, WITH -DUSE_TEAM_CODE to use the real code.

#pragma once

#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Report {
    int reportID = 0;
    std::string category;
    std::string itemName;
    std::string color;
    std::string location;
    int dateFound = 0;
    int score = 0;
};

struct LostItem {
    std::string category;
    std::string itemName;
    std::string color;
    std::string location;
    int dateLost = 0;
};

using ScoredReport = Report;
using Query = LostItem;

// Higher score first; ties broken by smaller reportID first.
inline bool ranksBefore(const ScoredReport& a, const ScoredReport& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.reportID < b.reportID;
}

// CSV: reportID,category,itemName,color,location,dateFound  (header row skipped)
inline std::vector<Report> loadReports(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open file: " + path);
    std::vector<Report> out;
    std::string line;
    std::getline(in, line);  // header
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string f[6];
        bool ok = true;
        for (int i = 0; i < 6; i++) {
            if (!std::getline(ss, f[i], ',')) { ok = false; break; }
        }
        if (!ok) continue;
        Report r;
        r.reportID = std::stoi(f[0]);
        r.category = f[1];
        r.itemName = f[2];
        r.color = f[3];
        r.location = f[4];
        r.dateFound = std::stoi(f[5]);
        out.push_back(std::move(r));
    }
    return out;
}

// Synthetic data with the same kinds of values as the real dataset.
inline std::vector<Report> generateReports(int n, unsigned seed = 42) {
    static const char* cats[] = {"electronics", "clothing", "bags", "accessories", "school supplies", "other"};
    static const char* items[] = {"phone", "laptop", "jacket", "backpack", "wallet", "keys", "textbook", "umbrella"};
    static const char* colors[] = {"black", "white", "blue", "red", "green", "gray", "brown", "pink"};
    static const char* locs[] = {"library west", "the hub", "reitz union", "turlington hall",
                                 "newell hall", "student rec", "marston science library", "plaza of the americas"};
    std::mt19937 rng(seed);
    std::vector<Report> v;
    v.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; i++) {
        Report r;
        r.reportID = i + 1;
        r.category = cats[rng() % 6];
        r.itemName = items[rng() % 8];
        r.color = colors[rng() % 8];
        r.location = locs[rng() % 8];
        r.dateFound = 1 + static_cast<int>(rng() % 365);
        v.push_back(std::move(r));
    }
    return v;
}

// Same rules as the team's calculateScore.
inline int calculateScore(const LostItem& lost, const Report& found) {
    int s = 0;
    if (lost.category == found.category) s += 4;
    if (lost.itemName == found.itemName) s += 3;
    if (lost.color == found.color) s += 2;
    if (lost.location == found.location) s += 2;
    if (found.dateFound >= lost.dateLost && found.dateFound <= lost.dateLost + 7) s += 1;
    return s;
}

inline std::vector<ScoredReport> scoreReports(const std::vector<Report>& reports, const Query& q) {
    LostItem lost = q;
    if (lost.dateLost <= 0) lost.dateLost = -1000;  // "no date given": never earns the date point
    std::vector<ScoredReport> out = reports;
    for (ScoredReport& r : out) r.score = calculateScore(lost, r);
    return out;
}

// ---------- merge sort (counts its own comparisons) ----------
inline void mergeSortImpl(std::vector<Report>& a, std::vector<Report>& tmp, size_t lo, size_t hi,
                          unsigned long long& cmp) {
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    mergeSortImpl(a, tmp, lo, mid, cmp);
    mergeSortImpl(a, tmp, mid, hi, cmp);
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        ++cmp;
        if (ranksBefore(a[j], a[i])) tmp[k++] = std::move(a[j++]);
        else tmp[k++] = std::move(a[i++]);
    }
    while (i < mid) tmp[k++] = std::move(a[i++]);
    while (j < hi) tmp[k++] = std::move(a[j++]);
    for (size_t x = lo; x < hi; x++) a[x] = std::move(tmp[x]);
}

inline unsigned long long runMergeSort(std::vector<ScoredReport>& v) {
    unsigned long long cmp = 0;
    std::vector<Report> tmp(v.size());
    mergeSortImpl(v, tmp, 0, v.size(), cmp);
    return cmp;
}

// ---------- quicksort (median-of-three, Hoare partition, counts comparisons) ----------
inline void quickSortImpl(std::vector<Report>& a, long lo, long hi, unsigned long long& cmp) {
    auto less = [&cmp](const Report& x, const Report& y) { ++cmp; return ranksBefore(x, y); };
    while (lo < hi) {
        long mid = lo + (hi - lo) / 2;
        if (less(a[mid], a[lo])) std::swap(a[mid], a[lo]);
        if (less(a[hi], a[lo])) std::swap(a[hi], a[lo]);
        if (less(a[hi], a[mid])) std::swap(a[hi], a[mid]);
        Report pivot = a[mid];
        long i = lo, j = hi;
        while (i <= j) {
            while (less(a[i], pivot)) i++;
            while (less(pivot, a[j])) j--;
            if (i <= j) { std::swap(a[i], a[j]); i++; j--; }
        }
        if (j - lo < hi - i) { quickSortImpl(a, lo, j, cmp); lo = i; }
        else { quickSortImpl(a, i, hi, cmp); hi = j; }
    }
}

inline unsigned long long runQuickSort(std::vector<ScoredReport>& v) {
    unsigned long long cmp = 0;
    if (v.size() > 1) quickSortImpl(v, 0, static_cast<long>(v.size()) - 1, cmp);
    return cmp;
}
