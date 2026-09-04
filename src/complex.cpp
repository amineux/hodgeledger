#include "hodgeledger/complex.hpp"

#include "hodgeledger/csv.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_set>

namespace hodgeledger {
namespace {

int parse_year(std::string_view s) {
    if (s.empty()) {
        return 0;
    }
    try {
        return std::stoi(std::string(s));
    } catch (...) {
        return 0;
    }
}

void add_undirected_edge(SimplicialComplex& K, int a, int b) {
    if (a == b || a < 0 || b < 0) {
        return;
    }
    const long long key = pack_uv(a, b);
    if (K.edge_of.count(key)) {
        return;
    }
    Edge e;
    e.u = std::min(a, b);
    e.v = std::max(a, b);
    const int ei = static_cast<int>(K.edges.size());
    K.edges.push_back(e);
    K.edge_of[key] = ei;
    K.adj[static_cast<std::size_t>(a)].push_back(b);
    K.adj[static_cast<std::size_t>(b)].push_back(a);
}

} // namespace

int SimplicialComplex::find_id(const std::string& id) const {
    const auto it = id_of.find(id);
    return it == id_of.end() ? -1 : it->second;
}

int SimplicialComplex::edge_index(int u, int v) const {
    const auto it = edge_of.find(pack_uv(u, v));
    return it == edge_of.end() ? -1 : it->second;
}

std::vector<int> SimplicialComplex::components() const {
    const int n = n0();
    std::vector<int> comp(static_cast<std::size_t>(n), -1);
    int cid = 0;
    for (int s = 0; s < n; ++s) {
        if (comp[static_cast<std::size_t>(s)] >= 0) {
            continue;
        }
        std::queue<int> q;
        q.push(s);
        comp[static_cast<std::size_t>(s)] = cid;
        while (!q.empty()) {
            const int u = q.front();
            q.pop();
            for (int v : adj[static_cast<std::size_t>(u)]) {
                if (comp[static_cast<std::size_t>(v)] < 0) {
                    comp[static_cast<std::size_t>(v)] = cid;
                    q.push(v);
                }
            }
        }
        ++cid;
    }
    return comp;
}

int SimplicialComplex::component_count() const {
    if (n0() == 0) {
        return 0;
    }
    const auto c = components();
    int m = -1;
    for (int v : c) {
        m = std::max(m, v);
    }
    return m + 1;
}

void assemble_boundaries(SimplicialComplex& K) {
    const int n0 = K.n0();
    const int n1 = K.n1();
    const int n2 = K.n2();
    K.degree.assign(static_cast<std::size_t>(n0), 0);
    for (int u = 0; u < n0; ++u) {
        K.degree[static_cast<std::size_t>(u)] = static_cast<int>(K.adj[static_cast<std::size_t>(u)].size());
    }

    std::vector<Triplet> b1;
    b1.reserve(static_cast<std::size_t>(2 * n1));
    for (int e = 0; e < n1; ++e) {
        const int u = K.edges[static_cast<std::size_t>(e)].u;
        const int v = K.edges[static_cast<std::size_t>(e)].v;
        b1.push_back({u, e, -1.0});
        b1.push_back({v, e, 1.0});
    }
    K.B1 = sparse_from_triplets(n0, n1, std::move(b1));
    K.B1t = sparse_transpose(K.B1);

    std::vector<Triplet> b2;
    b2.reserve(static_cast<std::size_t>(3 * n2));
    for (int t = 0; t < n2; ++t) {
        const Triangle& T = K.triangles[static_cast<std::size_t>(t)];
        const int e_ab = K.edge_index(T.a, T.b);
        const int e_bc = K.edge_index(T.b, T.c);
        const int e_ac = K.edge_index(T.a, T.c);
        if (e_ab < 0 || e_bc < 0 || e_ac < 0) {
            throw std::runtime_error("triangle is not a clique of the 1-skeleton");
        }
        // ∂[a,b,c] = [b,c] − [a,c] + [a,b], a<b<c, edges oriented low→high.
        b2.push_back({e_ab, t, 1.0});
        b2.push_back({e_bc, t, 1.0});
        b2.push_back({e_ac, t, -1.0});
    }
    K.B2 = sparse_from_triplets(n1, n2, std::move(b2));
    K.B2t = sparse_transpose(K.B2);
}

void infer_clique_triangles(SimplicialComplex& K) {
    K.triangles.clear();
    const int n = K.n0();
    std::vector<char> mark(static_cast<std::size_t>(n), 0);
    std::vector<int> stamped;
    for (int u = 0; u < n; ++u) {
        auto& nbrs = K.adj[static_cast<std::size_t>(u)];
        std::sort(nbrs.begin(), nbrs.end());
        nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
    }
    for (int u = 0; u < n; ++u) {
        const auto& nu = K.adj[static_cast<std::size_t>(u)];
        for (int v : nu) {
            if (v <= u) {
                continue;
            }
            for (int w : K.adj[static_cast<std::size_t>(v)]) {
                mark[static_cast<std::size_t>(w)] = 1;
                stamped.push_back(w);
            }
            for (int w : nu) {
                if (w <= v) {
                    continue;
                }
                if (mark[static_cast<std::size_t>(w)]) {
                    K.triangles.push_back({u, v, w});
                }
            }
            for (int w : stamped) {
                mark[static_cast<std::size_t>(w)] = 0;
            }
            stamped.clear();
        }
    }
}

SimplicialComplex load_complex(const std::string& papers_csv, const std::string& citations_csv,
                               const std::string& categories_csv, const std::string& triangles_csv) {
    SimplicialComplex K;
    const CsvTable papers = read_csv(papers_csv);
    K.papers.reserve(papers.rows.size());
    for (const auto& row : papers.rows) {
        Paper p;
        p.id = std::string(papers.get(row, "id"));
        if (p.id.empty()) {
            continue;
        }
        p.title = std::string(papers.get(row, "title"));
        p.year = parse_year(papers.get(row, "year"));
        p.category = std::string(papers.get(row, "category"));
        p.field = std::string(papers.get(row, "field"));
        p.authors = std::string(papers.get(row, "authors"));
        if (K.id_of.count(p.id)) {
            throw std::runtime_error("duplicate paper id: " + p.id);
        }
        K.id_of[p.id] = static_cast<int>(K.papers.size());
        K.papers.push_back(std::move(p));
    }
    const int n = K.n0();
    K.adj.assign(static_cast<std::size_t>(n), {});

    if (!categories_csv.empty()) {
        try {
            const CsvTable cats = read_csv(categories_csv);
            for (const auto& row : cats.rows) {
                Category c;
                c.id = std::string(cats.get(row, "id"));
                c.name = std::string(cats.get(row, "name"));
                c.group = std::string(cats.get(row, "group"));
                if (!c.id.empty()) {
                    K.categories.push_back(std::move(c));
                }
            }
        } catch (const std::exception&) {
            // optional file
        }
    }

    const CsvTable cites = read_csv(citations_csv);
    for (const auto& row : cites.rows) {
        const std::string a = std::string(cites.get(row, "citing"));
        const std::string b = std::string(cites.get(row, "cited"));
        if (a.empty() || b.empty() || a == b) {
            continue;
        }
        const int ia = K.find_id(a);
        const int ib = K.find_id(b);
        if (ia < 0 || ib < 0) {
            continue; // dangling skipped, never invented
        }
        DirectedCite d;
        d.citing = ia;
        d.cited = ib;
        d.year = parse_year(cites.get(row, "year"));
        if (d.year <= 0) {
            d.year = std::max(K.papers[static_cast<std::size_t>(ia)].year,
                              K.papers[static_cast<std::size_t>(ib)].year);
        }
        K.directed.push_back(d);
        add_undirected_edge(K, ia, ib);
    }

    if (!triangles_csv.empty()) {
        try {
            const CsvTable tris = read_csv(triangles_csv);
            std::unordered_set<long long> seen;
            for (const auto& row : tris.rows) {
                const int ia = K.find_id(std::string(tris.get(row, "a")));
                const int ib = K.find_id(std::string(tris.get(row, "b")));
                const int ic = K.find_id(std::string(tris.get(row, "c")));
                if (ia < 0 || ib < 0 || ic < 0) {
                    continue;
                }
                Triangle T{ia, ib, ic};
                if (T.a > T.b) {
                    std::swap(T.a, T.b);
                }
                if (T.b > T.c) {
                    std::swap(T.b, T.c);
                }
                if (T.a > T.b) {
                    std::swap(T.a, T.b);
                }
                if (T.a == T.b || T.b == T.c || T.a == T.c) {
                    continue;
                }
                if (K.edge_index(T.a, T.b) < 0 || K.edge_index(T.b, T.c) < 0 ||
                    K.edge_index(T.a, T.c) < 0) {
                    continue;
                }
                const long long key = (static_cast<long long>(T.a) << 42) |
                                      (static_cast<long long>(T.b) << 21) | T.c;
                if (!seen.insert(key).second) {
                    continue;
                }
                K.triangles.push_back(T);
            }
        } catch (const std::exception&) {
            infer_clique_triangles(K);
        }
    } else {
        infer_clique_triangles(K);
    }

    assemble_boundaries(K);
    return K;
}

namespace {

SimplicialComplex remap_keep(const SimplicialComplex& K, const std::vector<char>& keep_v,
                             const std::vector<char>& keep_e, const std::vector<char>& keep_t) {
    SimplicialComplex out;
    std::vector<int> vmap(static_cast<std::size_t>(K.n0()), -1);
    for (int i = 0; i < K.n0(); ++i) {
        if (!keep_v[static_cast<std::size_t>(i)]) {
            continue;
        }
        vmap[static_cast<std::size_t>(i)] = static_cast<int>(out.papers.size());
        out.papers.push_back(K.papers[static_cast<std::size_t>(i)]);
        out.id_of[out.papers.back().id] = vmap[static_cast<std::size_t>(i)];
    }
    out.categories = K.categories;
    out.adj.assign(static_cast<std::size_t>(out.n0()), {});
    for (int e = 0; e < K.n1(); ++e) {
        if (!keep_e[static_cast<std::size_t>(e)]) {
            continue;
        }
        const int u = vmap[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].u)];
        const int v = vmap[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].v)];
        if (u < 0 || v < 0) {
            continue;
        }
        const int nu = std::min(u, v);
        const int nv = std::max(u, v);
        const int ei = static_cast<int>(out.edges.size());
        out.edges.push_back({nu, nv});
        out.edge_of[pack_uv(nu, nv)] = ei;
        out.adj[static_cast<std::size_t>(nu)].push_back(nv);
        out.adj[static_cast<std::size_t>(nv)].push_back(nu);
    }
    for (int t = 0; t < K.n2(); ++t) {
        if (!keep_t[static_cast<std::size_t>(t)]) {
            continue;
        }
        const Triangle& T = K.triangles[static_cast<std::size_t>(t)];
        const int a = vmap[static_cast<std::size_t>(T.a)];
        const int b = vmap[static_cast<std::size_t>(T.b)];
        const int c = vmap[static_cast<std::size_t>(T.c)];
        if (a < 0 || b < 0 || c < 0) {
            continue;
        }
        Triangle N{a, b, c};
        if (N.a > N.b) {
            std::swap(N.a, N.b);
        }
        if (N.b > N.c) {
            std::swap(N.b, N.c);
        }
        if (N.a > N.b) {
            std::swap(N.a, N.b);
        }
        if (out.edge_index(N.a, N.b) < 0 || out.edge_index(N.b, N.c) < 0 || out.edge_index(N.a, N.c) < 0) {
            continue;
        }
        out.triangles.push_back(N);
    }
    for (const DirectedCite& d : K.directed) {
        const int a = vmap[static_cast<std::size_t>(d.citing)];
        const int b = vmap[static_cast<std::size_t>(d.cited)];
        if (a < 0 || b < 0) {
            continue;
        }
        out.directed.push_back({a, b, d.year});
    }
    assemble_boundaries(out);
    return out;
}

} // namespace

SimplicialComplex SimplicialComplex::without_edge(int edge_index) const {
    if (edge_index < 0 || edge_index >= n1()) {
        throw std::runtime_error("without_edge: index out of range");
    }
    std::vector<char> keep_v(static_cast<std::size_t>(n0()), 1);
    std::vector<char> keep_e(static_cast<std::size_t>(n1()), 1);
    std::vector<char> keep_t(static_cast<std::size_t>(n2()), 1);
    keep_e[static_cast<std::size_t>(edge_index)] = 0;
    const int u = edges[static_cast<std::size_t>(edge_index)].u;
    const int v = edges[static_cast<std::size_t>(edge_index)].v;
    for (int t = 0; t < n2(); ++t) {
        const Triangle& T = triangles[static_cast<std::size_t>(t)];
        const bool hit = (T.a == u && T.b == v) || (T.b == u && T.c == v) || (T.a == u && T.c == v);
        if (hit) {
            keep_t[static_cast<std::size_t>(t)] = 0;
        }
    }
    return remap_keep(*this, keep_v, keep_e, keep_t);
}

SimplicialComplex SimplicialComplex::without_triangle(int tri_index) const {
    if (tri_index < 0 || tri_index >= n2()) {
        throw std::runtime_error("without_triangle: index out of range");
    }
    std::vector<char> keep_v(static_cast<std::size_t>(n0()), 1);
    std::vector<char> keep_e(static_cast<std::size_t>(n1()), 1);
    std::vector<char> keep_t(static_cast<std::size_t>(n2()), 1);
    keep_t[static_cast<std::size_t>(tri_index)] = 0;
    return remap_keep(*this, keep_v, keep_e, keep_t);
}

SimplicialComplex SimplicialComplex::until_year(int year) const {
    std::vector<char> keep_v(static_cast<std::size_t>(n0()), 0);
    for (int i = 0; i < n0(); ++i) {
        if (papers[static_cast<std::size_t>(i)].year <= year) {
            keep_v[static_cast<std::size_t>(i)] = 1;
        }
    }
    std::vector<char> keep_e(static_cast<std::size_t>(n1()), 0);
    for (const DirectedCite& d : directed) {
        if (d.year <= year && keep_v[static_cast<std::size_t>(d.citing)] &&
            keep_v[static_cast<std::size_t>(d.cited)]) {
            const int e = edge_index(d.citing, d.cited);
            if (e >= 0) {
                keep_e[static_cast<std::size_t>(e)] = 1;
            }
        }
    }
    std::vector<char> keep_t(static_cast<std::size_t>(n2()), 0);
    for (int t = 0; t < n2(); ++t) {
        const Triangle& T = triangles[static_cast<std::size_t>(t)];
        const int e1 = edge_index(T.a, T.b);
        const int e2 = edge_index(T.b, T.c);
        const int e3 = edge_index(T.a, T.c);
        if (keep_v[static_cast<std::size_t>(T.a)] && keep_v[static_cast<std::size_t>(T.b)] &&
            keep_v[static_cast<std::size_t>(T.c)] && e1 >= 0 && e2 >= 0 && e3 >= 0 &&
            keep_e[static_cast<std::size_t>(e1)] && keep_e[static_cast<std::size_t>(e2)] &&
            keep_e[static_cast<std::size_t>(e3)]) {
            keep_t[static_cast<std::size_t>(t)] = 1;
        }
    }
    return remap_keep(*this, keep_v, keep_e, keep_t);
}

} // namespace hodgeledger
