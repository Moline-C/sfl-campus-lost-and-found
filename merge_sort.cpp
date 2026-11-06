#include "merge_sort.h"
#include <cstddef>

//if true left report is first
bool leftComesFirst(const Report& left, const Report& right) {
    if (left.score != right.score) {
        return left.score > right.score;
    }
    //if scores tie and left id is smaller or equal to right's, left goes first
    return left.reportID <= right.reportID;
}

// takes begin-mid and mid-end ranges and copies them into reports using temp to help with posititoning
// comparisions increases every time two reports are compared, we can compare this to the quicksort's
void mergeReports(vector<Report>& reports, vector<Report>& temporary, size_t begin, size_t middle, size_t end, int& comparisons) {
    size_t left = begin;
    size_t right = middle;
    size_t position = begin;
    while (left < middle && right < end) {
        ++comparisons;
        if (leftComesFirst(reports[left], reports[right])) {
            temporary[position] = reports[left];
            ++left;
        } else {
            temporary[position] = reports[right];
            ++right;
        }
        ++position;
    }
    while (left < middle) {
        temporary[position] = reports[left];
        ++left;
        ++position;
    }
    while (right < end) {
        temporary[position] = reports[right];
        ++right;
        ++position;
    }
    for (size_t i = begin; i < end; ++i) {
        reports[i] = temporary[i];
    }
}

//sorts begin-mid and mid-end recursively and then uses mergeReports to combine halves
void mergeSortRange(vector<Report>& reports, vector<Report>& temporary, size_t begin, size_t end, int& comparisons) {
    if (end - begin <= 1) {
        return;
    }
    size_t middle = begin + (end - begin) / 2;
    mergeSortRange(
        reports, temporary, begin, middle, comparisons
    );
    mergeSortRange(
        reports, temporary, middle, end, comparisons
    );
    mergeReports(
        reports, temporary, begin, middle, end, comparisons
    );
}

//main function to call and use in main.cpp
void mergeSort(vector<Report>& reports, int& comparisons) {
    comparisons = 0;
    vector<Report> temporary(reports.size());
    mergeSortRange(
        reports, temporary, 0, reports.size(), comparisons
    );
}