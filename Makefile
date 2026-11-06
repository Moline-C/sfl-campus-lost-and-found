CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

.PHONY: all generate sample

all: generate_reports

generate_reports: generate_reports.cpp
	$(CXX) $(CXXFLAGS) generate_reports.cpp -o generate_reports

generate: generate_reports
	./generate_reports 100000 reports.csv

sample: generate_reports
	./generate_reports 20 sample_reports.csv