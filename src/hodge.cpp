#include "hodgeledger/hodge.hpp"

#include <cmath>
#include <stdexcept>

namespace hodgeledger {

SparseMatrix hodge_L0(const SimplicialComplex& K) { return gram_aat(K.B1); }

SparseMatrix hodge_L1(const SimplicialComplex& K) {
    SparseMatrix down = gram_ata(K.B1);
    if (K.n2() == 0) {
        return down;
    }
    SparseMatrix up = gram_aat(K.B2);
    return sparse_add(down, up);
}

SparseMatrix hodge_L2(const SimplicialComplex& K) {
    if (K.n2() == 0) {
        return sparse_from_triplets(0, 0, {});
    }
    return gram_ata(K.B2);
}

SparseMatrix normalized_graph_laplacian(const SimplicialComplex& K) {
    const int n = K.n0();
    std::vector<double> invsqrt(static_cast<std::size_t>(n), 0.0);
    for (int i = 0; i < n; ++i) {
        const int d = K.degree[static_cast<std::size_t>(i)];
        if (d > 0) {
            invsqrt[static_cast<std::size_t>(i)] = 1.0 / std::sqrt(static_cast<double>(d));
        }
    }
    std::vector<Triplet> trips;
    trips.reserve(static_cast<std::size_t>(n + 2 * K.n1()));
    for (int i = 0; i < n; ++i) {
        if (K.degree[static_cast<std::size_t>(i)] > 0) {
            trips.push_back({i, i, 1.0});
        }
    }
    for (const Edge& e : K.edges) {
        const double s = -invsqrt[static_cast<std::size_t>(e.u)] * invsqrt[static_cast<std::size_t>(e.v)];
        trips.push_back({e.u, e.v, s});
        trips.push_back({e.v, e.u, s});
    }
    return sparse_from_triplets(n, n, std::move(trips));
}

} // namespace hodgeledger
