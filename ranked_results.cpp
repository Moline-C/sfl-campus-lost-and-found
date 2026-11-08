#include "ranked_results.h"
#include "matching.h"
#include "quick_sort.h"

vector<Report> getTopMatches(vector<Report> reports, LostItem lost) {
    for(int i = 0; i < reports.size(); i++) {
        reports[i].score = calculateScore(lost, reports[i]);
    }

    long long comparisons = 0;
    quickSort(reports, comparisons);

    vector<Report> topMatches;

    int limit = 10;

    if(reports.size() < 10) {
        limit = reports.size();
    }

    for(int i = 0; i < limit; i++) {
        topMatches.push_back(reports[i]);
    }

    return topMatches;
}
