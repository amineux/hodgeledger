#include "hodgeledger/complex.hpp"
#include "hodgeledger/hodge.hpp"
#include "hodgeledger/sparse.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

using namespace hodgeledger;

namespace {

SimplicialComplex triangle_complex() {
    // Three papers, three edges, one filled triangle.
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    return load_complex(dir + "/papers.csv", dir + "/citations.csv", dir + "/categories.csv",
                        dir + "/triangles.csv");
}

} // namespace

TEST(Complex, LoadsTiny) {
    auto K = triangle_complex();
    EXPECT_EQ(K.n0(), 12);
    EXPECT_GE(K.n1(), 16);
    EXPECT_GE(K.n2(), 8);
}

TEST(Complex, BoundarySquaredIsZero) {
    auto K = triangle_complex();
    // B1 B2 = 0
    std::vector<double> x(static_cast<std::size_t>(K.n2()), 0.0);
    if (K.n2() == 0) {
        GTEST_SKIP();
    }
    x[0] = 1.0;
    std::vector<double> y(static_cast<std::size_t>(K.n1()), 0.0);
    std::vector<double> z(static_cast<std::size_t>(K.n0()), 0.0);
    K.B2.multiply(x, y);
    K.B1.multiply(y, z);
    double n2 = 0.0;
    for (double v : z) {
        n2 += v * v;
    }
    EXPECT_NEAR(n2, 0.0, 1e-12);
}

TEST(Complex, WithoutEdgeDropsIncidentTriangles) {
    auto K = triangle_complex();
    const int n2 = K.n2();
    auto K2 = K.without_edge(0);
    EXPECT_EQ(K2.n1(), K.n1() - 1);
    EXPECT_LE(K2.n2(), n2);
}
