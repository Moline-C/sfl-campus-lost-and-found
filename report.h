#ifndef REPORT_H
#define REPORT_H
#include <string>
using namespace std;

struct Report {
    int reportID = 0;
    string category;
    string itemName;
    string color;
    string location;
    int dateFound = 0;
    int score = 0;
};

struct LostItem {
    string category;
    string itemName;
    string color;
    string location;
    int dateLost = 0;
};

#endif