#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

struct ItemCategory {
    string name;
    vector<string> items;
};

int main(int argc, char* argv[]) {
    int reportCount = 100000;
    string filename = "reports.csv";

    if (argc > 3) {
        cerr << "Usage: " << argv[0] << " [report_count] [output.csv]\n";
        return 1;
    }

    if (argc >= 2) {
        try {
            const string input = argv[1];
            if (input.empty() || input.find_first_not_of("0123456789") != string::npos) {
                throw invalid_argument("not a positive integer");
            }
            const long long count = stoll(input);
            if (count < 1 || count > numeric_limits<int>::max()) {
                throw out_of_range("report count");
            }
            reportCount = static_cast<int>(count);
        } catch (const exception&) {
            cerr << "Report count must be an integer from 1 to "
                      << numeric_limits<int>::max() << ".\n";
            return 1;
        }
    }
    if (argc == 3) {
        filename = argv[2];
    }
    const vector<ItemCategory> categories = {
        {"electronics", {"phone", "laptop", "tablet", "headphones", "charger"}},
        {"clothing", {"jacket", "hoodie", "hat", "scarf", "sweater"}},
        {"accessories", {"wallet", "watch", "sunglasses", "bracelet", "ring"}},
        {"bags", {"backpack", "tote bag", "purse", "duffel bag", "lunch bag"}},
        {"school supplies", {"notebook", "textbook", "binder", "pencil case", "calculator"}},
        {"other", {"water bottle", "umbrella", "keys", "travel mug", "yoga mat"}}
    };
    const vector<string> colors = {
        "black", "white", "blue", "red", "green", "gray", "brown", "pink", "purple", "yellow"
    };
    const vector<string> locations = {
        "library west", "marston science library", "reitz union",
        "turlington hall", "newell hall", "southwest recreation center",
        "student rec", "the hub", "plaza of the americas"
    };
    constexpr unsigned int seed = 42;
    mt19937 rng(seed);
    uniform_int_distribution<int> categoryIndex(0, static_cast<int>(categories.size()) - 1);
    uniform_int_distribution<int> colorIndex(0, static_cast<int>(colors.size()) - 1);
    uniform_int_distribution<int> locationIndex(0, static_cast<int>(locations.size()) - 1);


    uniform_int_distribution<int> dateFound(1, 365);

    ofstream output(filename);
    if (!output) {
        cerr << "Could not open output file: " << filename << '\n';
        return 1;
    }
    output << "reportID,category,itemName,color,location,dateFound\n";

    for (int i = 0; i < reportCount; ++i) {
        const ItemCategory& category = categories[categoryIndex(rng)];
        uniform_int_distribution<int> itemIndex(0, static_cast<int>(category.items.size()) - 1);
        const string& item = category.items[itemIndex(rng)];
        const string& color = colors[colorIndex(rng)];
        const string& location = locations[locationIndex(rng)];
        const int day = dateFound(rng);

        output << i + 1 << ',' << category.name << ',' << item << ','
               << color << ',' << location << ',' << day << '\n';
    }

    output.close();
    if (!output) {
        cerr << "Failed while writing: " << filename << '\n';
        return 1;
    }
    cout << "Created " << reportCount << " synthetic reports in "
              << filename << " (seed " << seed << ").\n";
    return 0;
}
