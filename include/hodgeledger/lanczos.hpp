#pragma once

#include "hodgeledger/sparse.hpp"
#include "hodgeledger/types.hpp"

#include <vector>

namespace hodgeledger {

struct LanczosOptions {
    int k = 8;             // eigenpairs requested (smallest)
    int max_steps = 0;     // 0 = auto min(n, max(4k+16, 64))
    double residual_tol = 1e-8;
    unsigned seed = 20260904u;
    bool skip_trivial = false;
};

struct LanczosResult {
    std::vector<EigenPair> pairs; // ascending eigenvalues
    int steps = 0;
    int converged = 0;
    double max_residual = 0.0;
};

/// Symmetric Lanczos with double reorthogonalization, Jacobi Rayleigh–Ritz,
/// and a random restart when an invariant subspace is found (multiplicity).
LanczosResult lanczos_smallest(const SparseMatrix& A, const LanczosOptions& opt = {});

void jacobi_symmetric(std::vector<double>& A, int n, std::vector<double>& evals,
                      std::vector<double>& evecs, double tol = 1e-14, int max_sweeps = 80);

} // namespace hodgeledger
