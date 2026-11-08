#ifndef REPORT_H
#define REPORT_H

#include <string>

using namespace std;

struct Report {
    int reportID;
    string category;
    string itemName;
    string color;
    string location;
    int dateFound;
    int score;
};

struct LostItem {
    string category;
    string itemName;
    string color;
    string location;
    int dateLost;
};

#endif
