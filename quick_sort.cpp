#include "quick_sort.h"

bool comesBefore(Report a, Report b, long long& comparisons) {
    comparisons++;

    if(a.score > b.score) {
        return true;
    }

    if(a.score < b.score) {
        return false;
    }

    return a.reportID < b.reportID;
}

int partitionReports(vector<Report>& reports, int low, int high, long long& comparisons) {

    int middle = low + (high - low) / 2;
    Report middleReport = reports[middle];
    reports[middle] = reports[high];
    reports[high] = middleReport;

    Report pivot = reports[high];
    int i = low - 1;

    for(int j = low; j < high; j++) {
        if(comesBefore(reports[j], pivot, comparisons)) {
            i++;

            Report temp = reports[i];
            reports[i] = reports[j];
            reports[j] = temp;
        }
    }

    Report temp = reports[i + 1];
    reports[i + 1] = reports[high];
    reports[high] = temp;

    return i + 1;
}

void quickSortHelper(vector<Report>& reports, int low, int high, long long& comparisons) {
    if(low < high) {
        int pivot = partitionReports(reports, low, high, comparisons);

        quickSortHelper(reports, low, pivot - 1, comparisons);
        quickSortHelper(reports, pivot + 1, high, comparisons);
    }
}

void quickSort(vector<Report>& reports, long long& comparisons) {
    comparisons = 0;

    if(reports.size() > 1) {
        quickSortHelper(reports, 0, reports.size() - 1, comparisons);
    }
}
