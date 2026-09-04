#include "hodgeledger/complex.hpp"
#include "hodgeledger/harmonic.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <utility>

using namespace hodgeledger;

namespace {

std::set<std::pair<std::string, std::string>> planted_edges() {
    const std::string path = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny/planted.json";
    std::ifstream in(path);
    std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto cut = body.find("cycle_edges");
    if (cut != std::string::npos) {
        body = body.substr(cut);
    }
    std::set<std::pair<std::string, std::string>> edges;
    std::string a, b;
    bool have_a = false;
    for (std::size_t i = 0; i + 10 < body.size(); ++i) {
        if (body.compare(i, 6, "synth-") == 0) {
            std::string id = body.substr(i, 10);
            if (!have_a) {
                a = id;
                have_a = true;
            } else {
                b = id;
                if (a > b) {
                    std::swap(a, b);
                }
                if (a != b) {
                    edges.insert({a, b});
                }
                have_a = false;
            }
        }
    }
    return edges;
}

} // namespace

TEST(Harmonic, PlantedCycleRecovered) {
    const std::string dir = std::string(HODGELEDGER_FIXTURE_DIR) + "/tiny";
    auto K = load_complex(dir + "/papers.csv", dir + "/citations.csv", dir + "/categories.csv",
                          dir + "/triangles.csv");
    HarmonicOptions opt;
    opt.k = 6;
    opt.seed = 20260904;
    opt.max_cycles = 4;
    opt.support_frac = 0.25;
    const HarmonicResult H = extract_harmonic(K, opt);
    ASSERT_FALSE(H.cycles.empty());
    EXPECT_GE(H.betti1, 1);
    EXPECT_LT(H.lambda_min, 1e-4) << "planted C4 should be harmonic";

    const auto planted = planted_edges();
    ASSERT_EQ(planted.size(), 4u);

    const HarmonicCycle& C = H.cycles.front();
    std::set<std::pair<std::string, std::string>> recovered;
    for (const auto& e : C.support) {
        std::string u = K.papers[static_cast<std::size_t>(e.u)].id;
        std::string v = K.papers[static_cast<std::size_t>(e.v)].id;
        if (u > v) {
            std::swap(u, v);
        }
        recovered.insert({u, v});
    }
    int hits = 0;
    for (const auto& e : planted) {
        if (recovered.count(e)) {
            ++hits;
        }
    }
    EXPECT_EQ(hits, 4) << "harmonic 1-chain should concentrate on the planted C4";

    // Mass on planted edges should dominate.
    double plant_mass = 0, other = 0;
    for (int e = 0; e < K.n1(); ++e) {
        std::string u = K.papers[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].u)].id;
        std::string v = K.papers[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].v)].id;
        if (u > v) {
            std::swap(u, v);
        }
        const double w = std::abs(C.vector[static_cast<std::size_t>(e)]);
        if (planted.count({u, v})) {
            plant_mass += w;
        } else {
            other += w;
        }
    }
    EXPECT_GT(plant_mass, other) << "planted cycle should carry the majority of |f|";
}
