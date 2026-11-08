// resolved.h
// "Mark as collected": once the owner has verified an item in person, its
// report is removed from the database.
//
// The shared reports.csv is never edited. Removed report IDs are appended to
// resolved_reports.txt (one ID per line) and skipped the next time the
// program loads the reports. Delete resolved_reports.txt to bring them back.

#pragma once

#include <algorithm>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "backend.h"

// Reads the IDs saved in the file. A missing file means nothing was removed yet.
inline std::vector<int> loadResolvedIds(const std::string& path) {
    std::vector<int> ids;
    std::ifstream in(path);
    int id = 0;
    while (in >> id) ids.push_back(id);
    return ids;
}

// Adds one ID to the end of the file. Returns false if the file cannot be written.
inline bool appendResolvedId(const std::string& path, int id) {
    std::ofstream out(path, std::ios::app);
    if (!out) return false;
    out << id << "\n";
    return static_cast<bool>(out);
}

// Removes the report with this ID. The other reports keep their order.
// Returns false if there is no such report.
inline bool removeReportById(std::vector<Report>& reports, int id) {
    for (size_t i = 0; i < reports.size(); i++) {
        if (reports[i].reportID == id) {
            reports.erase(reports.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

// Removes every report whose ID is in the list. Returns how many were removed.
inline size_t removeResolved(std::vector<Report>& reports, const std::vector<int>& ids) {
    std::unordered_set<int> gone(ids.begin(), ids.end());
    size_t before = reports.size();
    reports.erase(std::remove_if(reports.begin(), reports.end(),
                                 [&gone](const Report& r) { return gone.count(r.reportID) > 0; }),
                  reports.end());
    return before - reports.size();
}
