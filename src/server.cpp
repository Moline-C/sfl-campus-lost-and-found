// server.cpp  (Junia: browser interface)
//
// Starts a small web server on this computer (127.0.0.1 only) and serves the
// page in web/index.html. The page talks to the same C++ code as the terminal
// menu: scoring, merge sort, quicksort, and the timing/comparison counting.
//
//   GET  /api/options                 values for the drop-down lists
//   GET  /api/search?category=..&item=..&color=..&location=..&day=..&algo=merge|quick
//   GET  /api/compare                 runs the standard sort comparison
//   POST /api/resolve?id=..           removes a report once its item was collected
//   POST /api/quit                    stops the server
//
// Usage (run from the repository root so web/ is found):
//   ./lostfound_web [path/to/reports.csv] [port]      (defaults: reports.csv, 8080)
//
// Uses cpp-httplib (MIT license, header-only): third_party/httplib.h

#include "httplib.h"  // keep first: it includes the Windows socket headers

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "backend.h"
#include "benchmark.h"
#include "resolved.h"

// small helpers
static std::string lowerTrim(const std::string& in) {
    size_t b = in.find_first_not_of(" \t\r\n");
    size_t e = in.find_last_not_of(" \t\r\n");
    std::string s = (b == std::string::npos) ? "" : in.substr(b, e - b + 1);
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

static std::string q(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }

static std::string num(double v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << v;
    return os.str();
}

static std::string stringArray(const std::set<std::string>& values) {
    std::string out = "[";
    bool first = true;
    for (const std::string& v : values) {
        if (!first) out += ",";
        out += q(v);
        first = false;
    }
    return out + "]";
}

static void sendJson(httplib::Response& res, const std::string& body, int status = 200) {
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(body, "application/json");
}

static void openBrowser(const std::string& url) {
#ifdef _WIN32
    std::system(("start \"\" \"" + url + "\"").c_str());
#elif defined(__APPLE__)
    std::system(("open \"" + url + "\"").c_str());
#else
    (void)url;  // on Linux just print the address
#endif
}

int main(int argc, char** argv) {
    std::string path = (argc > 1) ? argv[1] : "reports.csv";
    int port = (argc > 2) ? std::atoi(argv[2]) : 8080;
    if (port < 1 || port > 65535) port = 8080;

    std::vector<Report> reports;
    try {
        reports = loadReports(path);
    } catch (const std::exception& e) {
        std::cout << "Could not load '" << path << "': " << e.what() << "\n";
#ifndef USE_TEAM_CODE
        std::cout << "Using 100,000 generated sample reports instead.\n";
        reports = generateReports(100000, 42);
#else
        std::cout << "Usage: lostfound_web path/to/reports.csv [port]\n";
        return 1;
#endif
    }
    if (reports.empty()) {
        std::cout << "No reports found in '" << path << "'.\n";
        return 1;
    }
    // Reports that were marked as collected in an earlier run stay removed.
    const std::string resolvedPath = "resolved_reports.txt";
    size_t alreadyCollected = removeResolved(reports, loadResolvedIds(resolvedPath));
    std::cout << "Loaded " << reports.size() << " reports.\n";
    if (alreadyCollected > 0) {
        std::cout << alreadyCollected << " reports were marked as collected earlier and are hidden.\n";
    }

    // Values for the drop-down lists.
    std::set<std::string> categories, items, colors, locations;
    for (const Report& r : reports) {
        categories.insert(r.category);
        items.insert(r.itemName);
        colors.insert(r.color);
        locations.insert(r.location);
    }

    // Only one sort job at a time, so timings are not distorted by another request.
    std::mutex sortLock;

    httplib::Server svr;

    if (!svr.set_mount_point("/", "web")) {
        std::cout << "Could not find the 'web' folder. Run this program from the repository root.\n";
        return 1;
    }

    svr.Get("/api/options", [&](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> guard(sortLock);
        std::string body = "{\"count\":" + std::to_string(reports.size()) +
                           ",\"categories\":" + stringArray(categories) +
                           ",\"items\":" + stringArray(items) +
                           ",\"colors\":" + stringArray(colors) +
                           ",\"locations\":" + stringArray(locations) + "}";
        sendJson(res, body);
    });

    svr.Get("/api/search", [&](const httplib::Request& req, httplib::Response& res) {
        Query query;
        query.category = lowerTrim(req.get_param_value("category"));
        query.itemName = lowerTrim(req.get_param_value("item"));
        query.color = lowerTrim(req.get_param_value("color"));
        query.location = lowerTrim(req.get_param_value("location"));
        query.dateLost = 0;
        try {
            std::string day = lowerTrim(req.get_param_value("day"));
            if (!day.empty()) query.dateLost = std::max(0, std::min(365, std::stoi(day)));
        } catch (...) {
            query.dateLost = 0;
        }
        bool useQuick = lowerTrim(req.get_param_value("algo")) == "quick";

        std::vector<ScoredReport> ranked;
        SortRun run;
        {
            std::lock_guard<std::mutex> guard(sortLock);
            std::vector<ScoredReport> scored = scoreReports(reports, query);
            run = useQuick ? timeQuickSort(scored, ranked) : timeMergeSort(scored, ranked);
        }

        std::string matches = "[";
        size_t shown = 0;
        for (const ScoredReport& r : ranked) {
            if (shown == 10 || r.score <= 0) break;  // ranked is sorted, so the rest score lower
            if (shown > 0) matches += ",";
            matches += "{\"rank\":" + std::to_string(shown + 1) +
                       ",\"reportID\":" + std::to_string(r.reportID) +
                       ",\"score\":" + std::to_string(r.score) +
                       ",\"category\":" + q(r.category) + ",\"item\":" + q(r.itemName) +
                       ",\"color\":" + q(r.color) + ",\"location\":" + q(r.location) +
                       ",\"day\":" + std::to_string(r.dateFound) + "}";
            shown++;
        }
        matches += "]";

        std::string body = "{\"algorithm\":" + q(run.name) + ",\"milliseconds\":" + num(run.milliseconds) +
                           ",\"comparisons\":" + std::to_string(run.comparisons) +
                           ",\"total\":" + std::to_string(reports.size()) +
                           ",\"matches\":" + matches + "}";
        sendJson(res, body);
    });

    svr.Get("/api/compare", [&](const httplib::Request&, httplib::Response& res) {
        std::string rows = "[";
        bool allSame = true;
        {
            std::lock_guard<std::mutex> guard(sortLock);
            bool first = true;
            allSame = runStandardComparison(reports, [&](const CompareRow& r) {
                if (!first) rows += ",";
                first = false;
                rows += "{\"label\":" + q(r.label) + ",\"size\":" + std::to_string(r.size) +
                        ",\"mergeMs\":" + num(r.merge.milliseconds) +
                        ",\"mergeComparisons\":" + std::to_string(r.merge.comparisons) +
                        ",\"quickMs\":" + num(r.quick.milliseconds) +
                        ",\"quickComparisons\":" + std::to_string(r.quick.comparisons) +
                        ",\"same\":" + (r.same ? "true" : "false") + "}";
            });
        }
        rows += "]";
        sendJson(res, "{\"allSame\":" + std::string(allSame ? "true" : "false") + ",\"rows\":" + rows + "}");
    });

    // Mark as collected: the owner verified the item in person, so remove its report.
    svr.Post("/api/resolve", [&](const httplib::Request& req, httplib::Response& res) {
        int id = 0;
        try {
            id = std::stoi(req.get_param_value("id"));
        } catch (...) {
            sendJson(res, "{\"removed\":false,\"error\":\"id must be a number\"}", 400);
            return;
        }
        std::lock_guard<std::mutex> guard(sortLock);
        if (!removeReportById(reports, id)) {
            sendJson(res, "{\"removed\":false,\"error\":\"no report with that id\"}", 404);
            return;
        }
        bool saved = appendResolvedId(resolvedPath, id);
        sendJson(res, "{\"removed\":true,\"remaining\":" + std::to_string(reports.size()) +
                          ",\"saved\":" + (saved ? "true" : "false") + "}");
    });

    svr.Post("/api/quit", [&](const httplib::Request&, httplib::Response& res) {
        sendJson(res, "{\"stopped\":true}");
        std::thread([&svr]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            svr.stop();
        }).detach();
    });

    // Local only: other computers on the network cannot reach this server.
    if (!svr.bind_to_port("127.0.0.1", port)) {
        std::cout << "Could not use port " << port << ". Try another one, e.g. ./lostfound_web " << path
                  << " 8081\n";
        return 1;
    }
    std::string url = "http://localhost:" + std::to_string(port);
    std::cout << "Lost-and-Found is running at " << url << "\n"
              << "Use the Quit button in the page (or press Ctrl+C here) to stop it.\n";
    openBrowser(url);
    svr.listen_after_bind();
    std::cout << "Goodbye!\n";
    return 0;
}
