#include "hodgeledger/sparse.hpp"

#include <gtest/gtest.h>

using namespace hodgeledger;

TEST(Sparse, IdentityMatvec) {
    auto I = sparse_from_triplets(3, 3, {{0, 0, 1}, {1, 1, 1}, {2, 2, 1}});
    std::vector<double> x = {2, 3, 4};
    std::vector<double> y(3);
    I.multiply(x, y);
    EXPECT_EQ(y[0], 2);
    EXPECT_EQ(y[1], 3);
    EXPECT_EQ(y[2], 4);
    EXPECT_DOUBLE_EQ(I.quadratic_form(x), 2 * 2 + 3 * 3 + 4 * 4);
}

TEST(Sparse, GramAndTranspose) {
    auto A = sparse_from_triplets(2, 3, {{0, 0, 1}, {0, 1, -1}, {1, 1, 1}, {1, 2, -1}});
    auto At = sparse_transpose(A);
    EXPECT_EQ(At.rows, 3);
    EXPECT_EQ(At.cols, 2);
    auto G = gram_aat(A);
    EXPECT_EQ(G.rows, 2);
    std::vector<double> x = {1, 0};
    std::vector<double> y(2);
    G.multiply(x, y);
    EXPECT_NEAR(y[0], 2.0, 1e-12);
}

TEST(Sparse, CoalesceZeros) {
    auto A = sparse_from_triplets(2, 2, {{0, 1, 1}, {0, 1, -1}, {1, 1, 3}});
    EXPECT_EQ(A.nnz(), 1);
}
