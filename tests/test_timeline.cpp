#include "hodgeledger/complex.hpp"
#include "hodgeledger/timeline.hpp"

#include <gtest/gtest.h>

using namespace hodgeledger;

TEST(Timeline, CumulativeGrows) {
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    auto K = load_complex(dir + "/papers.csv", dir + "/citations.csv", dir + "/categories.csv",
                          dir + "/triangles.csv");
    auto slices = compute_timeline(K, 20260904u, 24);
    ASSERT_FALSE(slices.empty());
    for (std::size_t i = 1; i < slices.size(); ++i) {
        EXPECT_GE(slices[i].n, slices[i - 1].n);
        EXPECT_GE(slices[i].year, slices[i - 1].year);
    }
    EXPECT_EQ(slices.back().n, K.n0());
    EXPECT_GE(slices.back().betti1, 1);
}
