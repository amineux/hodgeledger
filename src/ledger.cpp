#include "hodgeledger/ledger.hpp"

#include "hodgeledger/csv.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace hodgeledger {
namespace {

std::string pad_left(const std::string& s, int w, char ch = '0') {
    if (static_cast<int>(s.size()) >= w) {
        return s.substr(s.size() - static_cast<std::size_t>(w));
    }
    return std::string(static_cast<std::size_t>(w) - s.size(), ch) + s;
}

std::string pad_right(std::string s, int w) {
    if (static_cast<int>(s.size()) > w) {
        s.resize(static_cast<std::size_t>(w));
        return s;
    }
    s.append(static_cast<std::size_t>(w) - s.size(), ' ');
    return s;
}

std::string chart_acct(int dim, const std::string& paper_id) {
    return std::to_string(dim) + ":" + paper_id;
}

int posting_date(int year) { return year * 10000 + 615; }

} // namespace

Ledger post_ledger(const SimplicialComplex& K, const HarmonicResult& harm) {
    Ledger L;
    int je = 0;

    for (const DirectedCite& d : K.directed) {
        JournalLine line;
        line.je_id = ++je;
        const int y = d.year > 0 ? d.year : 2020;
        line.date = posting_date(y);
        line.dim = 0;
        line.debit = chart_acct(0, K.papers[static_cast<std::size_t>(d.citing)].id);
        line.credit = chart_acct(0, K.papers[static_cast<std::size_t>(d.cited)].id);
        line.amount_cents = 100;
        line.memo = "0-flow citation";
        L.journal.push_back(std::move(line));
    }

    if (!harm.cycles.empty()) {
        const HarmonicCycle& C = harm.cycles.front();
        double maxabs = 0.0;
        for (const auto& e : C.support) {
            maxabs = std::max(maxabs, std::abs(e.weight));
        }
        const double scale = maxabs > 0.0 ? 100.0 / maxabs : 0.0;
        for (const auto& e : C.support) {
            long long amt = static_cast<long long>(std::llround(std::abs(e.weight) * scale));
            if (amt <= 0) {
                amt = 1;
            }
            int src = e.u;
            int dst = e.v;
            if (e.weight < 0.0) {
                std::swap(src, dst);
            }
            JournalLine line;
            line.je_id = ++je;
            const int y = std::max(K.papers[static_cast<std::size_t>(src)].year,
                                   K.papers[static_cast<std::size_t>(dst)].year);
            line.date = posting_date(y > 0 ? y : 2020);
            line.dim = 1;
            line.debit = chart_acct(1, K.papers[static_cast<std::size_t>(src)].id);
            line.credit = chart_acct(1, K.papers[static_cast<std::size_t>(dst)].id);
            line.amount_cents = amt;
            line.memo = "1-flow harmonic";
            L.journal.push_back(std::move(line));
        }
    }

    std::map<std::string, Account> acc;
    auto bump = [&](const std::string& id, int dim, bool debit, long long amt) {
        Account& a = acc[id];
        a.id = id;
        a.dim = dim;
        if (id.size() >= 2 && id[1] == ':') {
            a.paper_id = id.substr(2);
        } else {
            a.paper_id = id;
        }
        if (debit) {
            a.debit_cents += amt;
        } else {
            a.credit_cents += amt;
        }
    };

    for (const JournalLine& j : L.journal) {
        bump(j.debit, j.dim, true, j.amount_cents);
        bump(j.credit, j.dim, false, j.amount_cents);
        L.total_debit += j.amount_cents;
        L.total_credit += j.amount_cents;
        if (j.dim == 0) {
            L.total_debit_0 += j.amount_cents;
            L.total_credit_0 += j.amount_cents;
        } else {
            L.total_debit_1 += j.amount_cents;
            L.total_credit_1 += j.amount_cents;
        }
    }
    for (auto& kv : acc) {
        kv.second.net_cents = kv.second.credit_cents - kv.second.debit_cents;
        L.accounts.push_back(kv.second);
    }
    std::sort(L.accounts.begin(), L.accounts.end(), [](const Account& a, const Account& b) {
        if (a.dim != b.dim) {
            return a.dim < b.dim;
        }
        return a.id < b.id;
    });
    L.balanced = (L.total_debit == L.total_credit) && (L.total_debit_0 == L.total_credit_0) &&
                 (L.total_debit_1 == L.total_credit_1);
    return L;
}

void write_journal_csv(const std::string& path, const Ledger& led) {
    std::vector<std::vector<std::string>> rows;
    rows.reserve(led.journal.size());
    for (const auto& j : led.journal) {
        rows.push_back({std::to_string(j.je_id), std::to_string(j.date), std::to_string(j.dim), j.debit,
                        j.credit, std::to_string(j.amount_cents), j.memo});
    }
    write_csv(path, {"je_id", "date", "dim", "debit_account", "credit_account", "amount_cents", "memo"},
              rows);
}

void write_flows_csv(const std::string& path, const Ledger& led) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& j : led.journal) {
        rows.push_back({std::to_string(j.dim), j.debit, j.credit, std::to_string(j.date / 10000),
                        std::to_string(j.amount_cents), j.memo});
    }
    write_csv(path, {"dim", "debit", "credit", "year", "amount_cents", "memo"}, rows);
}

void write_journal_dat(const std::string& path, const Ledger& led) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot write journal.dat: " + path);
    }
    for (const auto& j : led.journal) {
        std::string rec;
        rec += pad_left(std::to_string(j.je_id), 8);
        rec += pad_left(std::to_string(j.date), 8);
        rec += std::to_string(j.dim % 10);
        rec += ' ';
        rec += pad_right(j.debit, 16);
        rec += pad_right(j.credit, 16);
        rec += pad_left(std::to_string(j.amount_cents), 10);
        rec += pad_right(j.memo, 36);
        if (static_cast<int>(rec.size()) != kJournalWidth) {
            rec.resize(static_cast<std::size_t>(kJournalWidth), ' ');
        }
        out << rec << '\n';
    }
}

void write_trial_balance_csv(const std::string& path, const Ledger& led) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& a : led.accounts) {
        rows.push_back({std::to_string(a.dim), a.id, std::to_string(a.debit_cents),
                        std::to_string(a.credit_cents), std::to_string(a.net_cents)});
    }
    rows.push_back({"", "TOTAL", std::to_string(led.total_debit), std::to_string(led.total_credit),
                    std::to_string(led.total_credit - led.total_debit)});
    write_csv(path, {"dim", "account", "debit_cents", "credit_cents", "net_cents"}, rows);
}

bool verify_journal(const Ledger& led, std::string* err) {
    auto fail = [&](const std::string& m) {
        if (err) {
            *err = m;
        }
        return false;
    };
    long long d = 0, c = 0, d0 = 0, c0 = 0, d1 = 0, c1 = 0;
    for (const auto& j : led.journal) {
        if (j.debit.empty() || j.credit.empty() || j.debit == j.credit) {
            return fail("accounts must be distinct and nonempty");
        }
        if (j.amount_cents <= 0) {
            return fail("amount must be positive");
        }
        if (j.dim != 0 && j.dim != 1) {
            return fail("dim must be 0 or 1");
        }
        d += j.amount_cents;
        c += j.amount_cents;
        if (j.dim == 0) {
            d0 += j.amount_cents;
            c0 += j.amount_cents;
        } else {
            d1 += j.amount_cents;
            c1 += j.amount_cents;
        }
    }
    if (d != c || d0 != c0 || d1 != c1) {
        return fail("trial balance does not zero");
    }
    return true;
}

} // namespace hodgeledger
