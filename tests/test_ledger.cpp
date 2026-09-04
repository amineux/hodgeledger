#include "hodgeledger/complex.hpp"
#include "hodgeledger/harmonic.hpp"
#include "hodgeledger/ledger.hpp"

#include <fstream>
#include <gtest/gtest.h>

using namespace hodgeledger;

TEST(Ledger, DoubleEntryAndTwoChartsBalance) {
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    auto K = load_complex(dir + "/papers.csv", dir + "/citations.csv", dir + "/categories.csv",
                          dir + "/triangles.csv");
    HarmonicResult H = extract_harmonic(K, {});
    Ledger L = post_ledger(K, H);
    std::string err;
    ASSERT_TRUE(verify_journal(L, &err)) << err;
    EXPECT_TRUE(L.balanced);
    EXPECT_EQ(L.total_debit, L.total_credit);
    EXPECT_EQ(L.total_debit_0, L.total_credit_0);
    EXPECT_EQ(L.total_debit_1, L.total_credit_1);
    EXPECT_GT(L.total_debit_0, 0);
    EXPECT_GT(L.total_debit_1, 0);

    bool saw0 = false, saw1 = false;
    for (const auto& a : L.accounts) {
        if (a.dim == 0) {
            saw0 = true;
        }
        if (a.dim == 1) {
            saw1 = true;
        }
        EXPECT_EQ(a.net_cents, a.credit_cents - a.debit_cents);
    }
    EXPECT_TRUE(saw0);
    EXPECT_TRUE(saw1);
}

TEST(Ledger, DatWidth96) {
    JournalLine j;
    j.je_id = 1;
    j.date = 20180615;
    j.dim = 0;
    j.debit = "0:synth-0001";
    j.credit = "0:synth-0002";
    j.amount_cents = 100;
    j.memo = "0-flow citation";
    Ledger L;
    L.journal.push_back(j);
    const std::string path = "/tmp/hodgeledger-journal-test.dat";
    write_journal_dat(path, L);
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    EXPECT_EQ(static_cast<int>(line.size()), kJournalWidth);
}
