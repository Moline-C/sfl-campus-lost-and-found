#include "load_csv.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

//string to positive int 
int readPositiveInteger(const string& text) {
    if (text.empty() ||
        text.find_first_not_of("0123456789") != string::npos) {
        throw runtime_error("Expected a positive integer.");
    }

    int value = stoi(text);

    if (value <= 0) {
        throw runtime_error("Expected a positive integer.");
    }

    return value;
}

//remove carriage return
void removeCarriageReturn(string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}
//reads csv line by line, creates report object and adds to vector
vector<Report> loadReports(const string& filename) {
    ifstream input(filename);

    if (!input.is_open()) {
        throw runtime_error("Cannot open file: " + filename);
    }
    vector<Report> reports;
    unordered_set<int> usedIDs;
    string line;
    getline(input, line);
    removeCarriageReturn(line);
    if (line != "reportID,category,itemName,color,location,dateFound") {
        throw runtime_error("Missing or incorrect CSV header.");
    }
    int lineNumber = 1;
    while (getline(input, line)) {
        ++lineNumber;
        removeCarriageReturn(line);

        try {
            if (line.empty() || line.back() == ',') {
                throw runtime_error("Missing CSV fields.");
            }
            stringstream row(line);
            vector<string> fields;
            string field;
            while (getline(row, field, ',')) {
                fields.push_back(field);
            }

            if (fields.size() != 6) {
                throw runtime_error("Expected exactly six fields.");
            }

            for (const string& value : fields) {
                if (value.empty()) {
                    throw runtime_error(
                        "Fields cannot be empty"
                    );
                }
            }

            Report report;
            report.reportID = readPositiveInteger(fields[0]);
            report.category = fields[1];
            report.itemName = fields[2];
            report.color = fields[3];
            report.location = fields[4];
            report.dateFound = readPositiveInteger(fields[5]);


            if (!usedIDs.insert(report.reportID).second) {
                throw runtime_error("Duplicate report ID.");
            }

            reports.push_back(report);
        } catch (const exception& error) {
            throw runtime_error(
                "CSV line " + to_string(lineNumber) +
                ": " + error.what()
            );
        }
    }

    if (input.bad()) {
        throw runtime_error("Error in: " + filename);
    }

    return reports;
}