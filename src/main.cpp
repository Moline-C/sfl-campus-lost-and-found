// main.cpp  (Junia: menu and results display)
//
// Screens: Main Menu -> Search -> Results, plus "Compare algorithms".
// Usage:  ./lostfound [path/to/reports.csv]     (default: reports.csv)

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "backend.h"
#include "benchmark.h"

// ---------- input helpers ----------
static std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Prints the prompt and reads one trimmed, lowercase line (the dataset is all lowercase).
// Returns false only if input ended (Ctrl+D / closed pipe).
static bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    size_t b = out.find_first_not_of(" \t\r\n");
    size_t e = out.find_last_not_of(" \t\r\n");
    out = (b == std::string::npos) ? "" : toLower(out.substr(b, e - b + 1));
    return true;
}

// Reads a whole number in [lo, hi]. Returns false only if input ended.
static bool readInt(const std::string& prompt, int lo, int hi, int& out) {
    std::string line;
    while (true) {
        if (!readLine(prompt, line)) return false;
        try {
            size_t used = 0;
            int v = std::stoi(line, &used);
            if (used == line.size() && v >= lo && v <= hi) {
                out = v;
                return true;
            }
        } catch (...) {
        }
        std::cout << "  Please enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

// ---------- screens ----------
static void printMainMenu() {
    std::cout << "\n========================================\n"
              << "   Campus Lost-and-Found Tracker\n"
              << "========================================\n"
              << "  1. Find an item\n"
              << "  2. Compare sorting algorithms\n"
              << "  3. Exit\n";
}

static void printResults(const std::vector<ScoredReport>& ranked, const std::string& algo,
                         double ms, unsigned long long comparisons) {
    const size_t kTop = 10;
    std::cout << "\n------------------------------ Top matches ------------------------------\n";
    std::cout << std::left << std::setw(5) << "Rank" << std::setw(9) << "ReportID" << std::setw(7)
              << "Score" << std::setw(17) << "Category" << std::setw(16) << "Item" << std::setw(9)
              << "Color" << std::setw(29) << "Location" << "Day\n";

    size_t shown = 0;
    for (const ScoredReport& r : ranked) {
        if (shown == kTop || r.score <= 0) break;  // ranked is sorted, so the rest are worse
        std::cout << std::left << std::setw(5) << (shown + 1) << std::setw(9) << r.reportID
                  << std::setw(7) << r.score << std::setw(17) << r.category << std::setw(16)
                  << r.itemName << std::setw(9) << r.color << std::setw(29) << r.location
                  << r.dateFound << "\n";
        shown++;
    }
    if (shown == 0) {
        std::cout << "No matching reports found. Try fewer or different details.\n";
    }
    std::cout << "\nSorted with " << algo << " in " << std::fixed << std::setprecision(2) << ms
              << " ms (" << comparisons << " comparisons).\n"
              << "If one of these is yours, come in person to collect and verify it.\n";
}

static void searchScreen(const std::vector<Report>& reports) {
    Query q;
    std::cout << "\n--- Search for a lost item (press Enter to skip a field) ---\n";
    if (!readLine("Category (e.g. electronics, clothing, bags): ", q.category)) return;
    if (!readLine("Item name (e.g. phone, jacket, backpack): ", q.itemName)) return;
    if (!readLine("Color: ", q.color)) return;
    if (!readLine("Last known location (e.g. library west, the hub): ", q.location)) return;
    if (!readInt("Approximate day lost (1-365, 0 to skip): ", 0, 365, q.dateLost)) return;

    if (q.category.empty() && q.itemName.empty() && q.color.empty() && q.location.empty() &&
        q.dateLost == 0) {
        std::cout << "Enter at least one detail to search with.\n";
        return;
    }

    int choice = 1;
    if (!readInt("Sort with 1) Merge sort  2) Quicksort: ", 1, 2, choice)) return;

    std::vector<ScoredReport> scored = scoreReports(reports, q);
    std::vector<ScoredReport> ranked;
    SortRun run = (choice == 1) ? timeMergeSort(scored, ranked) : timeQuickSort(scored, ranked);
    printResults(ranked, run.name, run.milliseconds, run.comparisons);
}

static void compareScreen(const std::vector<Report>& reports) {
    std::cout << "\n--- Compare merge sort vs quicksort ---\n"
              << "Sample search: electronics / phone / black / library west / day 100\n"
              << "Both sorts get an identical copy of the same scored reports.\n\n";

    bool allSame = runStandardComparison(reports, [](const CompareRow& r) { printCompareRow(r); });

    std::cout << "\n" << (allSame ? "Both algorithms produced the same ranking every time."
                                  : "WARNING: the two algorithms disagreed on at least one run.")
              << "\n";
}

int main(int argc, char** argv) {
    std::string path = (argc > 1) ? argv[1] : "reports.csv";

    std::vector<Report> reports;
    try {
        reports = loadReports(path);
    } catch (const std::exception& e) {
        std::cout << "Could not load '" << path << "': " << e.what() << "\n";
#ifndef USE_TEAM_CODE
        std::cout << "Using 100,000 generated sample reports instead.\n";
        reports = generateReports(100000, 42);
#else
        std::cout << "Usage: ./lostfound path/to/reports.csv\n";
        return 1;
#endif
    }
    if (reports.empty()) {
        std::cout << "No reports found in '" << path << "'.\n";
        return 1;
    }
    std::cout << "Loaded " << reports.size() << " reports.\n";

    while (true) {
        printMainMenu();
        int choice = 0;
        if (!readInt("Choose an option (1-3): ", 1, 3, choice)) break;
        if (choice == 1) searchScreen(reports);
        else if (choice == 2) compareScreen(reports);
        else break;
    }
    std::cout << "Goodbye!\n";
    return 0;
}
