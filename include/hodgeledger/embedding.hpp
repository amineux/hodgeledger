#pragma once

#include "hodgeledger/complex.hpp"
#include "hodgeledger/lanczos.hpp"

#include <string>
#include <vector>

namespace hodgeledger {

struct NodeEmbed {
    int index = 0;
    int cluster = 0;
    int degree = 0;
    std::vector<double> x; // raw L0 coords (nontrivial)
    std::vector<double> u; // row-normalized
};

struct Embedding {
    int k = 0;
    double trivial_eigenvalue = 0.0;
    double algebraic_connectivity = 0.0;      // first nontrivial L0
    double fiedler_lambda2 = 0.0;             // ClaimLedger L_sym baseline
    std::vector<double> l0_eigenvalues;
    std::vector<double> l_sym_eigenvalues;
    std::vector<NodeEmbed> nodes;
    LanczosResult l0_lanczos;
    LanczosResult fiedler_lanczos;
};

Embedding embed_complex(const SimplicialComplex& K, int k, unsigned seed,
                        int lanczos_steps = 0);

std::vector<int> kmeans_pp(const std::vector<std::vector<double>>& unit_rows, int clusters,
                           unsigned seed, int iters = 40);

} // namespace hodgeledger
