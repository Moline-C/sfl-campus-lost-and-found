// test_sorts.cpp  (Junia)
//
// Checks merge sort and quicksort on:
//   - different dataset sizes (0, 1, 2, 10, 100, 1,000, 10,000, 100,000)
//   - different starting orders (random, already ranked, reverse ranked, nearly ranked)
//   - different score patterns (all unique, many repeats, only 3 scores, all the same)
// For every case it verifies that
//   1) each algorithm's output is correctly ranked (score high->low, ties by reportID),
//   2) merge sort and quicksort give the exact same ranking,
//   3) the ranking matches std::sort (used only as a trusted reference here).
//
// Build (from the repo root):
//   g++ -std=c++17 -O2 -Isrc tests/test_sorts.cpp -o test_sorts
// Run:
//   ./test_sorts       (exit code 0 = all tests passed)

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "backend.h"
#include "benchmark.h"

static int g_passed = 0;
static int g_failed = 0;

// Score patterns --------------------------------------------------------
enum class Pattern { Unique, ManyRepeats, ThreeScores, AllSame };

static const char* patternName(Pattern p) {
    switch (p) {
        case Pattern::Unique: return "unique scores";
        case Pattern::ManyRepeats: return "many repeated scores";
        case Pattern::ThreeScores: return "3 distinct scores";
        case Pattern::AllSame: return "all same score";
    }
    return "";
}

// Starting orders -------------------------------------------------------
enum class Order { Random, Ranked, Reverse, NearlyRanked };

static const char* orderName(Order o) {
    switch (o) {
        case Order::Random: return "random order";
        case Order::Ranked: return "already ranked";
        case Order::Reverse: return "reverse ranked";
        case Order::NearlyRanked: return "nearly ranked";
    }
    return "";
}

static std::vector<ScoredReport> makeData(size_t n, Pattern p, Order o, unsigned seed) {
    std::mt19937 rng(seed);
    std::vector<ScoredReport> v(n);
    for (size_t i = 0; i < n; i++) {
        v[i].reportID = static_cast<int>(i) + 1;  // IDs are unique
        switch (p) {
            case Pattern::Unique: v[i].score = static_cast<int>(i); break;
            case Pattern::ManyRepeats: v[i].score = static_cast<int>(rng() % 11); break;  // 0..10
            case Pattern::ThreeScores: v[i].score = static_cast<int>(rng() % 3); break;
            case Pattern::AllSame: v[i].score = 5; break;
        }
    }
    // Put into the requested starting order.
    std::shuffle(v.begin(), v.end(), rng);
    if (o != Order::Random) {
        std::sort(v.begin(), v.end(), ranksBefore);
        if (o == Order::Reverse) {
            std::reverse(v.begin(), v.end());
        } else if (o == Order::NearlyRanked && n > 1) {
            size_t swaps = std::max<size_t>(1, n / 100);  // disturb about 1%
            for (size_t k = 0; k < swaps; k++) std::swap(v[rng() % n], v[rng() % n]);
        }
    }
    return v;
}

static bool isRanked(const std::vector<ScoredReport>& v) {
    for (size_t i = 1; i < v.size(); i++) {
        if (ranksBefore(v[i], v[i - 1])) return false;  // v[i] should never beat v[i-1]
    }
    return true;
}

static void runCase(size_t n, Pattern p, Order o, unsigned seed) {
    std::vector<ScoredReport> input = makeData(n, p, o, seed);

    std::vector<ScoredReport> reference = input;
    std::sort(reference.begin(), reference.end(), ranksBefore);

    std::vector<ScoredReport> mergeOut, quickOut;
    SortRun m = timeMergeSort(input, mergeOut);
    SortRun q = timeQuickSort(input, quickOut);

    std::string label = "n=" + std::to_string(n) + ", " + patternName(p) + ", " + orderName(o);
    std::string problem;
    if (mergeOut.size() != n) problem += " [merge sort changed the size]";
    if (quickOut.size() != n) problem += " [quicksort changed the size]";
    if (!isRanked(mergeOut)) problem += " [merge sort output not ranked]";
    if (!isRanked(quickOut)) problem += " [quicksort output not ranked]";
    if (!sameRanking(mergeOut, reference)) problem += " [merge sort != reference]";
    if (!sameRanking(quickOut, reference)) problem += " [quicksort != reference]";
    if (!sameRanking(mergeOut, quickOut)) problem += " [merge sort != quicksort]";

    if (problem.empty()) {
        g_passed++;
        std::cout << "PASS  " << label << "  (merge " << m.comparisons << " cmp, quick "
                  << q.comparisons << " cmp)\n";
    } else {
        g_failed++;
        std::cout << "FAIL  " << label << problem << "\n";
    }
}

int main() {
    const Pattern patterns[] = {Pattern::Unique, Pattern::ManyRepeats, Pattern::ThreeScores,
                                Pattern::AllSame};
    const Order orders[] = {Order::Random, Order::Ranked, Order::Reverse, Order::NearlyRanked};
    const size_t sizes[] = {0, 1, 2, 3, 10, 100, 1000, 10000, 100000};

    std::cout << "=== Sorting tests: sizes x score patterns x starting orders ===\n";
    unsigned seed = 1;
    for (size_t n : sizes)
        for (Pattern p : patterns)
            for (Order o : orders) runCase(n, p, o, seed++);

    std::cout << "\nPassed: " << g_passed << "   Failed: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
