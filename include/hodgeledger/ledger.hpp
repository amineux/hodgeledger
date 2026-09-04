#pragma once

#include "hodgeledger/complex.hpp"
#include "hodgeledger/harmonic.hpp"
#include "hodgeledger/types.hpp"

#include <string>
#include <vector>

namespace hodgeledger {

struct Account {
    std::string id; // chart-prefixed, e.g. 0:synth-0001
    int dim = 0;
    std::string paper_id;
    long long debit_cents = 0;
    long long credit_cents = 0;
    long long net_cents = 0;
};

struct Ledger {
    bool balanced = false;
    long long total_debit = 0;
    long long total_credit = 0;
    long long total_debit_0 = 0;
    long long total_credit_0 = 0;
    long long total_debit_1 = 0;
    long long total_credit_1 = 0;
    std::vector<JournalLine> journal;
    std::vector<Account> accounts;
};

Ledger post_ledger(const SimplicialComplex& K, const HarmonicResult& harm);

void write_journal_csv(const std::string& path, const Ledger& led);
void write_journal_dat(const std::string& path, const Ledger& led);
void write_trial_balance_csv(const std::string& path, const Ledger& led);
void write_flows_csv(const std::string& path, const Ledger& led);

bool verify_journal(const Ledger& led, std::string* err = nullptr);

constexpr int kJournalWidth = 96;

} // namespace hodgeledger
