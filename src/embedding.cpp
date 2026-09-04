#include "hodgeledger/embedding.hpp"

#include "hodgeledger/hodge.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <unordered_set>

namespace hodgeledger {
namespace {

std::vector<double> row_normalize(const std::vector<double>& x) {
    double n2 = 0.0;
    for (double v : x) {
        n2 += v * v;
    }
    n2 = std::sqrt(n2);
    std::vector<double> u = x;
    if (n2 > 1e-15) {
        for (double& v : u) {
            v /= n2;
        }
    }
    return u;
}

int count_near_zero(const std::vector<EigenPair>& pairs, double tol = 1e-8) {
    int c = 0;
    for (const auto& p : pairs) {
        if (std::abs(p.value) < tol) {
            ++c;
        } else {
            break;
        }
    }
    return c;
}

} // namespace

std::vector<int> kmeans_pp(const std::vector<std::vector<double>>& unit_rows, int clusters,
                           unsigned seed, int iters) {
    const int n = static_cast<int>(unit_rows.size());
    if (n == 0) {
        return {};
    }
    const int k = std::max(1, std::min(clusters, n));
    const int dim = static_cast<int>(unit_rows[0].size());
    std::mt19937 rng(seed);
    std::vector<int> centers;
    centers.reserve(static_cast<std::size_t>(k));
    std::uniform_int_distribution<int> uni(0, n - 1);
    centers.push_back(uni(rng));
    std::vector<double> dist(static_cast<std::size_t>(n), std::numeric_limits<double>::infinity());
    auto dist2 = [&](int i, int c) {
        double s = 0.0;
        for (int d = 0; d < dim; ++d) {
            const double a = unit_rows[static_cast<std::size_t>(i)][static_cast<std::size_t>(d)] -
                             unit_rows[static_cast<std::size_t>(c)][static_cast<std::size_t>(d)];
            s += a * a;
        }
        return s;
    };
    while (static_cast<int>(centers.size()) < k) {
        double sum = 0.0;
        for (int i = 0; i < n; ++i) {
            dist[static_cast<std::size_t>(i)] =
                std::min(dist[static_cast<std::size_t>(i)], dist2(i, centers.back()));
            sum += dist[static_cast<std::size_t>(i)];
        }
        std::uniform_real_distribution<double> pick(0.0, std::max(sum, 1e-15));
        double r = pick(rng);
        int chosen = n - 1;
        for (int i = 0; i < n; ++i) {
            r -= dist[static_cast<std::size_t>(i)];
            if (r <= 0.0) {
                chosen = i;
                break;
            }
        }
        centers.push_back(chosen);
    }

    std::vector<std::vector<double>> C(static_cast<std::size_t>(k), std::vector<double>(static_cast<std::size_t>(dim), 0.0));
    for (int c = 0; c < k; ++c) {
        C[static_cast<std::size_t>(c)] = unit_rows[static_cast<std::size_t>(centers[static_cast<std::size_t>(c)])];
    }
    std::vector<int> assign(static_cast<std::size_t>(n), 0);
    for (int it = 0; it < iters; ++it) {
        bool changed = false;
        for (int i = 0; i < n; ++i) {
            double best = std::numeric_limits<double>::infinity();
            int bc = 0;
            for (int c = 0; c < k; ++c) {
                double s = 0.0;
                for (int d = 0; d < dim; ++d) {
                    const double a = unit_rows[static_cast<std::size_t>(i)][static_cast<std::size_t>(d)] -
                                     C[static_cast<std::size_t>(c)][static_cast<std::size_t>(d)];
                    s += a * a;
                }
                if (s < best) {
                    best = s;
                    bc = c;
                }
            }
            if (assign[static_cast<std::size_t>(i)] != bc) {
                assign[static_cast<std::size_t>(i)] = bc;
                changed = true;
            }
        }
        std::vector<std::vector<double>> sum(static_cast<std::size_t>(k),
                                             std::vector<double>(static_cast<std::size_t>(dim), 0.0));
        std::vector<int> cnt(static_cast<std::size_t>(k), 0);
        for (int i = 0; i < n; ++i) {
            const int c = assign[static_cast<std::size_t>(i)];
            ++cnt[static_cast<std::size_t>(c)];
            for (int d = 0; d < dim; ++d) {
                sum[static_cast<std::size_t>(c)][static_cast<std::size_t>(d)] +=
                    unit_rows[static_cast<std::size_t>(i)][static_cast<std::size_t>(d)];
            }
        }
        for (int c = 0; c < k; ++c) {
            if (cnt[static_cast<std::size_t>(c)] == 0) {
                C[static_cast<std::size_t>(c)] = unit_rows[static_cast<std::size_t>(uni(rng))];
                continue;
            }
            for (int d = 0; d < dim; ++d) {
                C[static_cast<std::size_t>(c)][static_cast<std::size_t>(d)] =
                    sum[static_cast<std::size_t>(c)][static_cast<std::size_t>(d)] /
                    static_cast<double>(cnt[static_cast<std::size_t>(c)]);
            }
        }
        if (!changed && it > 0) {
            break;
        }
    }
    return assign;
}

Embedding embed_complex(const SimplicialComplex& K, int k, unsigned seed, int lanczos_steps) {
    Embedding E;
    E.k = k;
    const SparseMatrix L0 = hodge_L0(K);
    const SparseMatrix Lsym = normalized_graph_laplacian(K);
    const int ncomp = std::max(1, K.component_count());
    const int want_l0 = std::min(K.n0(), std::max(k + ncomp + 2, ncomp + 1));

    LanczosOptions opt;
    opt.k = want_l0;
    opt.max_steps = lanczos_steps;
    opt.seed = seed;
    E.l0_lanczos = lanczos_smallest(L0, opt);

    LanczosOptions fopt = opt;
    fopt.k = std::min(K.n0(), ncomp + 4);
    fopt.seed = seed + 17u;
    E.fiedler_lanczos = lanczos_smallest(Lsym, fopt);

    for (const auto& p : E.l0_lanczos.pairs) {
        E.l0_eigenvalues.push_back(p.value);
    }
    for (const auto& p : E.fiedler_lanczos.pairs) {
        E.l_sym_eigenvalues.push_back(p.value);
    }
    if (!E.l0_eigenvalues.empty()) {
        E.trivial_eigenvalue = E.l0_eigenvalues.front();
    }
    const int drop = std::max(1, count_near_zero(E.l0_lanczos.pairs));
    if (static_cast<int>(E.l0_lanczos.pairs.size()) > drop) {
        E.algebraic_connectivity = E.l0_lanczos.pairs[static_cast<std::size_t>(drop)].value;
    }
    const int drop_f = std::max(1, count_near_zero(E.fiedler_lanczos.pairs));
    if (static_cast<int>(E.fiedler_lanczos.pairs.size()) > drop_f) {
        E.fiedler_lambda2 = E.fiedler_lanczos.pairs[static_cast<std::size_t>(drop_f)].value;
    } else if (!E.l_sym_eigenvalues.empty()) {
        E.fiedler_lambda2 = 0.0;
    }

    const int n = K.n0();
    const int dim = std::min(k, std::max(0, static_cast<int>(E.l0_lanczos.pairs.size()) - drop));
    E.k = dim;
    std::vector<std::vector<double>> units(static_cast<std::size_t>(n));
    E.nodes.resize(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        NodeEmbed& nd = E.nodes[static_cast<std::size_t>(i)];
        nd.index = i;
        nd.degree = K.degree[static_cast<std::size_t>(i)];
        nd.x.assign(static_cast<std::size_t>(dim), 0.0);
        for (int d = 0; d < dim; ++d) {
            nd.x[static_cast<std::size_t>(d)] =
                E.l0_lanczos.pairs[static_cast<std::size_t>(drop + d)].vector[static_cast<std::size_t>(i)];
        }
        nd.u = row_normalize(nd.x);
        units[static_cast<std::size_t>(i)] = nd.u.empty() ? std::vector<double>{0.0} : nd.u;
    }

    std::unordered_set<std::string> fields;
    for (const auto& p : K.papers) {
        fields.insert(p.field);
    }
    const int clusters = std::max(1, static_cast<int>(fields.size()));
    const auto labels = kmeans_pp(units, clusters, seed + 99u);
    for (int i = 0; i < n; ++i) {
        E.nodes[static_cast<std::size_t>(i)].cluster = labels[static_cast<std::size_t>(i)];
    }
    return E;
}

} // namespace hodgeledger
