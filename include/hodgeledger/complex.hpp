#pragma once

#include "hodgeledger/sparse.hpp"
#include "hodgeledger/types.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hodgeledger {

/// Oriented simplicial complex of papers (0), coupling edges (1), and
/// filled triangles (2). Edge orientation is combinatorial: u → v with u < v.
struct SimplicialComplex {
    std::vector<Paper> papers;
    std::vector<Category> categories;
    std::vector<DirectedCite> directed;
    std::vector<Edge> edges;
    std::vector<Triangle> triangles;

    std::unordered_map<std::string, int> id_of;
    std::vector<std::vector<int>> adj;          // undirected neighbor indices
    std::vector<int> degree;                    // undirected degree
    std::unordered_map<long long, int> edge_of; // pack_uv(u,v) → edge index

    SparseMatrix B1;  // |V| × |E|
    SparseMatrix B1t; // |E| × |V|
    SparseMatrix B2;  // |E| × |T|
    SparseMatrix B2t; // |T| × |E|

    [[nodiscard]] int n0() const { return static_cast<int>(papers.size()); }
    [[nodiscard]] int n1() const { return static_cast<int>(edges.size()); }
    [[nodiscard]] int n2() const { return static_cast<int>(triangles.size()); }

    [[nodiscard]] int find_id(const std::string& id) const;
    [[nodiscard]] int edge_index(int u, int v) const;
    [[nodiscard]] std::vector<int> components() const;
    [[nodiscard]] int component_count() const;

    /// Drop one 1-simplex and every 2-simplex that used it; rebuild boundaries.
    [[nodiscard]] SimplicialComplex without_edge(int edge_index) const;
    /// Drop one 2-simplex; rebuild B2 only (1-skeleton unchanged).
    [[nodiscard]] SimplicialComplex without_triangle(int tri_index) const;
    /// Cumulative time slice: papers with year ≤ t, citations/edges/triangles
    /// whose members all exist in the slice.
    [[nodiscard]] SimplicialComplex until_year(int year) const;
};

SimplicialComplex load_complex(const std::string& papers_csv, const std::string& citations_csv,
                               const std::string& categories_csv = {},
                               const std::string& triangles_csv = {});

void assemble_boundaries(SimplicialComplex& K);
void infer_clique_triangles(SimplicialComplex& K);

[[nodiscard]] inline long long pack_uv(int u, int v) {
    if (u > v) {
        std::swap(u, v);
    }
    return (static_cast<long long>(u) << 32) | static_cast<unsigned int>(v);
}

} // namespace hodgeledger
