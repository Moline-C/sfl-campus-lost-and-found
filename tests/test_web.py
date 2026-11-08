#!/usr/bin/env python3
"""Feature tests for the browser interface (Junia).

Start the web server first, then in another terminal run:

    python tests/test_web.py [port]          (default port 8080)

The tests call the same JSON endpoints the page uses, so they check every
feature of the interface: page, drop-down lists, search with both sorts,
comparison, and Mark as collected. Only the Python standard library is needed.

Note: the last group removes ONE report from the running database (that is the
feature being tested). To bring it back, stop the server, delete
resolved_reports.txt, and start the server again.
"""
import json
import sys
import urllib.error
import urllib.parse
import urllib.request

PORT = int(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1].isdigit() else 8080
BASE = f"http://localhost:{PORT}"
passed = 0
failed = 0


def call(path, method="GET"):
    """Returns (status, body text)."""
    req = urllib.request.Request(BASE + path, method=method, data=b"" if method == "POST" else None)
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            return resp.status, resp.read().decode("utf-8")
    except urllib.error.HTTPError as err:
        return err.code, err.read().decode("utf-8")


def api(path, method="GET"):
    status, body = call(path, method)
    return status, json.loads(body)


def check(ok, what):
    global passed, failed
    print(("  PASS  " if ok else "  FAIL  ") + what)
    if ok:
        passed += 1
    else:
        failed += 1


def feature(title):
    print("\n" + title)


def search(**params):
    return api("/api/search?" + urllib.parse.urlencode(params))


def main():
    try:
        status, page = call("/")
    except Exception as err:  # server not running
        print(f"Could not reach {BASE}: {err}\nStart the server first (lostfound_web).")
        return 1

    feature("W1  The page loads with the three menu items")
    check(status == 200 and "Campus Lost and Found" in page, "the page is served")
    check("Find an item" in page and "Compare sorting" in page and "Quit" in page, "menu has Find an item, Compare sorting and Quit")

    feature("W2  Drop-down lists come from the data")
    _, opts = api("/api/options")
    check(opts["count"] > 0, f"{opts['count']:,} reports are loaded")
    check(all(len(opts[k]) > 0 for k in ("categories", "items", "colors", "locations")), "category, item, color and location lists are filled")
    check(opts["categories"] == sorted(opts["categories"]), "lists are sorted")

    feature("W3  Search with merge sort (the demo search)")
    demo = dict(category="electronics", item="phone", color="black", location="library west", day="100", algo="merge")
    _, merge = search(**demo)
    ms = merge["matches"]
    check(merge["algorithm"] == "Merge sort", "merge sort was used")
    check(0 < len(ms) <= 10, f"{len(ms)} matches returned (at most 10)")
    check(all(ms[i]["score"] >= ms[i + 1]["score"] for i in range(len(ms) - 1)), "scores never increase down the list")
    check(all(not (ms[i]["score"] == ms[i + 1]["score"] and ms[i]["reportID"] > ms[i + 1]["reportID"]) for i in range(len(ms) - 1)),
          "equal scores are in report ID order")
    check(all(m["score"] > 0 for m in ms), "reports with score 0 are not shown")
    check(merge["milliseconds"] >= 0 and merge["comparisons"] > 0, f"time {merge['milliseconds']} ms and {merge['comparisons']:,} comparisons are reported")
    check(merge["total"] == opts["count"], "the whole database was searched")

    feature("W4  Search with quicksort gives the same ranking")
    _, quick = search(**dict(demo, algo="quick"))
    check(quick["algorithm"] == "Quicksort", "quicksort was used")
    check([(m["reportID"], m["score"]) for m in quick["matches"]] == [(m["reportID"], m["score"]) for m in ms],
          "same reports in the same order as merge sort")
    check(quick["milliseconds"] < 5000, f"quicksort finished in {quick['milliseconds']} ms")

    feature("W5  Any detail can be left out")
    _, only_cat = search(category="electronics", algo="merge")
    check(len(only_cat["matches"]) == 10 and all(m["category"] == "electronics" for m in only_cat["matches"]), "category only: top 10 are all electronics")
    _, only_day = search(day="100", algo="merge")
    check(only_day["matches"] and all(m["score"] == 1 and 100 <= m["day"] <= 107 for m in only_day["matches"]),
          "date only: matches were found 0 to 7 days after the loss date")
    _, only_loc = search(location="the hub", algo="quick")
    check(only_loc["matches"] and all(m["location"] == "the hub" for m in only_loc["matches"]), "location only (quicksort): top 10 are all at the hub")

    feature("W6  No match, odd input")
    _, none = search(category="no such category", algo="merge")
    check(none["matches"] == [], "an unknown category returns no matches")
    _, caps = search(category="  ELECTRONICS ", item="Phone", algo="merge")
    _, low = search(category="electronics", item="phone", algo="merge")
    check(caps["matches"] == low["matches"], "capital letters and extra spaces are ignored")
    status, bad_day = call("/api/search?" + urllib.parse.urlencode(dict(category="bags", day="abc", algo="quick")))
    check(status == 200, "a day that is not a number does not break the search")
    _, other_algo = search(category="bags", algo="nonsense")
    check(other_algo["algorithm"] == "Merge sort", "an unknown sort name falls back to merge sort")

    feature("W7  Compare sorting")
    _, cmp_ = api("/api/compare")
    rows = cmp_["rows"]
    labels = " | ".join(r["label"] for r in rows)
    check(len(rows) >= 7, f"{len(rows)} comparison rows")
    check(cmp_["allSame"] and all(r["same"] for r in rows), "both sorts gave the same ranking in every row")
    check(any(r["size"] == opts["count"] for r in rows), "the full database is one of the rows")
    check("size 1000" in labels and "size 10000" in labels and "size 50000" in labels, "smaller sizes are compared")
    check("Already ranked" in labels and "Reverse ranked" in labels and "3 distinct scores" in labels, "starting orders and repeated scores are compared")
    check(all(r["mergeMs"] >= 0 and r["quickMs"] >= 0 and r["mergeComparisons"] > 0 and r["quickComparisons"] > 0 for r in rows),
          "time and comparisons are reported for both sorts in every row")

    feature("W8  Mark as collected removes a report")
    _, hit = search(category="other", item="yoga mat", algo="merge")
    target = hit["matches"][0]["reportID"]
    status, res = api(f"/api/resolve?id={target}", "POST")
    check(status == 200 and res["removed"] is True, f"report {target} was removed")
    check(res["remaining"] == opts["count"] - 1, f"{res['remaining']:,} reports remain")
    _, after = search(category="other", item="yoga mat", algo="merge")
    check(all(m["reportID"] != target for m in after["matches"]), "the removed report no longer appears in search results")
    _, opts2 = api("/api/options")
    check(opts2["count"] == opts["count"] - 1, "the report count went down by one")
    status, _ = api(f"/api/resolve?id={target}", "POST")
    check(status == 404, "removing the same report again is refused")
    status, _ = api("/api/resolve?id=abc", "POST")
    check(status == 400, "an ID that is not a number is refused")
    print(f"\n  (Report {target} is now removed. To bring it back: stop the server, delete resolved_reports.txt, start it again.)")

    if "--quit" in sys.argv:
        feature("W9  Quit stops the server")
        status, res = api("/api/quit", "POST")
        check(status == 200 and res["stopped"] is True, "the server confirms it is stopping")

    print(f"\nPassed: {passed}   Failed: {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
