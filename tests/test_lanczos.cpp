#include "hodgeledger/hodge.hpp"
#include "hodgeledger/lanczos.hpp"
#include "hodgeledger/sparse.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

using namespace hodgeledger;

namespace {

SparseMatrix cycle_graph_laplacian(int n) {
    std::vector<Triplet> t;
    for (int i = 0; i < n; ++i) {
        t.push_back({i, i, 2.0});
        t.push_back({i, (i + 1) % n, -1.0});
        t.push_back({i, (i + n - 1) % n, -1.0});
    }
    return sparse_from_triplets(n, n, std::move(t));
}

SparseMatrix complete_laplacian(int n) {
    std::vector<Triplet> t;
    for (int i = 0; i < n; ++i) {
        t.push_back({i, i, static_cast<double>(n - 1)});
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                t.push_back({i, j, -1.0});
            }
        }
    }
    return sparse_from_triplets(n, n, std::move(t));
}

} // namespace

TEST(Lanczos, Cycle6Spectrum) {
    // Combinatorial Laplacian of C6: 0, 1, 1, 3, 3, 4.
    auto L = cycle_graph_laplacian(6);
    LanczosOptions opt;
    opt.k = 6;
    opt.max_steps = 6;
    opt.seed = 20260904;
    auto R = lanczos_smallest(L, opt);
    ASSERT_GE(static_cast<int>(R.pairs.size()), 6);
    const double expected[6] = {0, 1, 1, 3, 3, 4};
    for (int i = 0; i < 6; ++i) {
        EXPECT_NEAR(R.pairs[static_cast<std::size_t>(i)].value, expected[i], 1e-6) << "i=" << i;
    }
}

TEST(Lanczos, CompleteK5) {
    // K5 combinatorial Laplacian: 0 once, then 5 with multiplicity 4.
    auto L = complete_laplacian(5);
    LanczosOptions opt;
    opt.k = 5;
    opt.max_steps = 5;
    opt.seed = 7;
    auto R = lanczos_smallest(L, opt);
    ASSERT_GE(static_cast<int>(R.pairs.size()), 5);
    EXPECT_NEAR(R.pairs[0].value, 0.0, 1e-6);
    for (int i = 1; i < 5; ++i) {
        EXPECT_NEAR(R.pairs[static_cast<std::size_t>(i)].value, 5.0, 1e-4) << "i=" << i;
    }
}

TEST(Lanczos, ResidualSmall) {
    auto L = cycle_graph_laplacian(6);
    LanczosOptions opt;
    opt.k = 3;
    opt.seed = 1;
    auto R = lanczos_smallest(L, opt);
    EXPECT_LT(R.max_residual, 1e-6);
}
