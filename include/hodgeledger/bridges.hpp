#pragma once

#include "hodgeledger/complex.hpp"
#include "hodgeledger/harmonic.hpp"

#include <string>
#include <vector>

namespace hodgeledger {

struct Bridge {
    int rank = 0;
    int edge = -1;
    std::string id; // "u|v" paper ids
    std::string u_id;
    std::string v_id;
    double score = 0.0;
    double abs_flow = 0.0;
    double delta_lambda = 0.0;
    bool loo = false;
    std::string explanation;
    std::string kind; // "edge" or "triangle"
};

struct BridgeReport {
    std::string method;
    std::string prefilter;
    int loo_candidates = 0;
    int loo_evaluated = 0;
    double lambda_min_l1 = 0.0;
    std::vector<Bridge> bridges;
};

struct BridgeOptions {
    bool loo = true;
    int loo_candidates = 32;
    int top = 24;
    unsigned seed = 20260904u;
    int lanczos_steps = 0;
};

/// Leave-one-edge-out Δλ_min(L1) on a |harmonic flow| shortlist.
BridgeReport score_bridges(const SimplicialComplex& K, const HarmonicResult& harm,
                           const BridgeOptions& opt = {});

} // namespace hodgeledger
