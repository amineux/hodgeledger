#include "hodgeledger/bridges.hpp"
#include "hodgeledger/complex.hpp"
#include "hodgeledger/harmonic.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <set>
#include <string>

using namespace hodgeledger;

TEST(Bridges, LeaveOneEdgeOutKillsPlantedCycle) {
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    auto K = load_complex(dir + "/papers.csv", dir + "/citations.csv", dir + "/categories.csv",
                          dir + "/triangles.csv");
    HarmonicOptions hopt;
    hopt.k = 6;
    hopt.seed = 20260904;
    const HarmonicResult H = extract_harmonic(K, hopt);
    ASSERT_LT(H.lambda_min, 1e-4);

    BridgeOptions bopt;
    bopt.loo = true;
    bopt.loo_candidates = K.n1(); // exact on tiny
    bopt.top = K.n1();
    bopt.seed = 20260904;
    const BridgeReport B = score_bridges(K, H, bopt);
    ASSERT_FALSE(B.bridges.empty());
    EXPECT_EQ(B.method, "leave-one-edge-out");
    EXPECT_EQ(B.loo_evaluated, K.n1());

    // The unique maximizer of Δλ should be a planted-cycle edge: deleting it
    // destroys ker L1, so λ_min jumps from ~0 to a positive gap.
    const Bridge& top = B.bridges.front();
    EXPECT_TRUE(top.loo);
    EXPECT_GT(top.delta_lambda, 1e-4) << "removing a harmonic-cycle edge must lift λ_min(L1)";

    std::ifstream in(dir + "/planted.json");
    std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(body.find(top.u_id) != std::string::npos && body.find(top.v_id) != std::string::npos)
        << "top LOO edge should be on the planted cycle, got " << top.id;
}
