#pragma once

#include "hodgeledger/complex.hpp"
#include "hodgeledger/sparse.hpp"

namespace hodgeledger {

/// Combinatorial Hodge Laplacians:
///   L_0 = B1 B1^T
///   L_1 = B1^T B1 + B2 B2^T
///   L_2 = B2^T B2   (no 3-simplices)
SparseMatrix hodge_L0(const SimplicialComplex& K);
SparseMatrix hodge_L1(const SimplicialComplex& K);
SparseMatrix hodge_L2(const SimplicialComplex& K);

/// ClaimLedger graph-only baseline: symmetric normalized Laplacian of the
/// 0-skeleton, L_sym = I - D^{-1/2} A D^{-1/2}. Isolates get a zero row.
SparseMatrix normalized_graph_laplacian(const SimplicialComplex& K);

} // namespace hodgeledger
