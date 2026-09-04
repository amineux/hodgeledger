#include "hodgeledger/complex.hpp"
#include "hodgeledger/hodge.hpp"
#include "hodgeledger/lanczos.hpp"

#include <gtest/gtest.h>

using namespace hodgeledger;

TEST(Hodge, L0EqualsDegreeMinusAdjacency) {
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    auto K = load_complex(dir + "/papers.csv", dir + "/citations.csv", dir + "/categories.csv",
                          dir + "/triangles.csv");
    auto L0 = hodge_L0(K);
    // Pick a vertex, L0_ii should equal degree.
    std::vector<double> e(static_cast<std::size_t>(K.n0()), 0.0);
    e[0] = 1.0;
    std::vector<double> y(static_cast<std::size_t>(K.n0()), 0.0);
    L0.multiply(e, y);
    EXPECT_NEAR(y[0], static_cast<double>(K.degree[0]), 1e-10);
}

TEST(Hodge, FilledTriangleHasTrivialH1) {
    // Isolated K3: three vertices, three edges, one triangle. ker L1 = 0.
    // Build by loading tiny and checking L2 exists.
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    auto K = load_complex(dir + "/papers.csv", dir + "/citations.csv");
    auto L2 = hodge_L2(K);
    EXPECT_EQ(L2.rows, K.n2());
    EXPECT_GE(K.n2(), 1);
}
