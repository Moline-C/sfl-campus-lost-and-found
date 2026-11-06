#ifndef RANKED_RESULTS_H
#define RANKED_RESULTS_H

#include <vector>
#include "report.h"

using namespace std;

vector<Report> getTopMatches(vector<Report> reports, LostItem lost);

#endif
