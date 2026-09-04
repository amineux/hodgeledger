#pragma once

#include "hodgeledger/complex.hpp"

#include <string>
#include <vector>

namespace hodgeledger {

struct TimelineSlice {
    int year = 0;
    int n = 0;
    int m = 0;
    int triangles = 0;
    int components = 0;
    double lambda2_l0 = 0.0;
    double lambda_min_l1 = 0.0;
    int betti1 = 0;
    std::string top_cycle_hint;
};

std::vector<TimelineSlice> compute_timeline(const SimplicialComplex& K, unsigned seed = 20260904u,
                                            int lanczos_steps = 0);

} // namespace hodgeledger
