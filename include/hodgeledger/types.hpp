#pragma once

#include <string>
#include <vector>

namespace hodgeledger {

struct Paper {
    std::string id;
    std::string title;
    std::string category;
    std::string field;
    std::string authors;
    int year = 0;
};

struct Category {
    std::string id;
    std::string name;
    std::string group;
};

struct DirectedCite {
    int citing = -1;
    int cited = -1;
    int year = 0;
};

struct Edge {
    int u = -1; // oriented u → v with u < v
    int v = -1;
};

struct Triangle {
    int a = -1; // a < b < c
    int b = -1;
    int c = -1;
};

struct EigenPair {
    double value = 0.0;
    std::vector<double> vector;
};

struct JournalLine {
    int je_id = 0;
    int date = 0; // YYYYMMDD
    int dim = 0;  // 0-flow or 1-flow
    std::string debit;
    std::string credit;
    long long amount_cents = 0;
    std::string memo;
};

} // namespace hodgeledger
