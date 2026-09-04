#pragma once

#include "hodgeledger/complex.hpp"
#include "hodgeledger/lanczos.hpp"

#include <string>
#include <vector>

namespace hodgeledger {

struct HarmonicEdge {
    int edge = -1;
    int u = -1;
    int v = -1;
    double weight = 0.0; // signed 1-cochain value
};

struct HarmonicCycle {
    int rank = 0;
    double lambda = 0.0;
    double energy = 0.0;
    double residual = 0.0;
    double cross_field_mass = 0.0;
    double participation_entropy = 0.0;
    std::string pair_a;
    std::string pair_b;
    std::vector<HarmonicEdge> support;
    std::vector<std::string> paper_loop; // ordered ids if a simple cycle was traced
    std::vector<double> vector;          // full 1-cochain
};

struct HarmonicResult {
    int betti1 = 0;
    double lambda_min = 0.0;
    double harmonic_gap = 0.0; // first strictly positive L1 eval
    std::vector<double> l1_eigenvalues;
    std::vector<HarmonicCycle> cycles;
    LanczosResult lanczos;
    std::vector<std::vector<double>> edge_embedding; // L1 coords per edge
};

struct HarmonicOptions {
    int k = 8;
    int lanczos_steps = 0;
    unsigned seed = 20260904u;
    double harmonic_tol = 1e-4;
    double support_frac = 0.12;
    int max_cycles = 16;
};

HarmonicResult extract_harmonic(const SimplicialComplex& K, const HarmonicOptions& opt = {});

} // namespace hodgeledger
