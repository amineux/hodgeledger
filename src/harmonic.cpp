#include "hodgeledger/harmonic.hpp"

#include "hodgeledger/hodge.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace hodgeledger {
namespace {

double shannon(const std::unordered_map<std::string, double>& mass) {
    double tot = 0.0;
    for (const auto& kv : mass) {
        tot += kv.second;
    }
    if (tot <= 0.0) {
        return 0.0;
    }
    double H = 0.0;
    for (const auto& kv : mass) {
        const double p = kv.second / tot;
        if (p > 0.0) {
            H -= p * std::log(p);
        }
    }
    return H;
}

std::vector<std::string> trace_loop(const SimplicialComplex& K, const std::vector<HarmonicEdge>& support) {
    if (support.empty()) {
        return {};
    }
    std::unordered_map<int, std::vector<int>> adj;
    std::unordered_set<int> verts;
    for (const auto& e : support) {
        adj[e.u].push_back(e.v);
        adj[e.v].push_back(e.u);
        verts.insert(e.u);
        verts.insert(e.v);
    }
    int start = support.front().u;
    std::vector<std::string> loop;
    std::unordered_set<long long> used;
    int cur = start;
    loop.push_back(K.papers[static_cast<std::size_t>(cur)].id);
    for (int step = 0; step < static_cast<int>(support.size()) + 2; ++step) {
        int nxt = -1;
        for (int nb : adj[cur]) {
            const long long key = pack_uv(cur, nb);
            if (!used.count(key)) {
                nxt = nb;
                used.insert(key);
                break;
            }
        }
        if (nxt < 0) {
            break;
        }
        loop.push_back(K.papers[static_cast<std::size_t>(nxt)].id);
        cur = nxt;
        if (cur == start && loop.size() > 2) {
            break;
        }
    }
    return loop;
}

HarmonicCycle cycle_from_vector(const SimplicialComplex& K, const EigenPair& pair, double support_frac) {
    HarmonicCycle C;
    C.lambda = pair.value;
    C.energy = pair.value; // unit vector ⇒ Rayleigh = λ
    C.vector = pair.vector;
    double maxabs = 0.0;
    for (double v : pair.vector) {
        maxabs = std::max(maxabs, std::abs(v));
    }
    const double thr = std::max(1e-12, support_frac * maxabs);
    std::unordered_map<std::string, double> pair_mass;
    std::unordered_map<std::string, double> field_mass;
    double cross = 0.0;
    for (int e = 0; e < K.n1(); ++e) {
        const double w = pair.vector[static_cast<std::size_t>(e)];
        if (std::abs(w) < thr) {
            continue;
        }
        HarmonicEdge he;
        he.edge = e;
        he.u = K.edges[static_cast<std::size_t>(e)].u;
        he.v = K.edges[static_cast<std::size_t>(e)].v;
        he.weight = w;
        C.support.push_back(he);
        const std::string& fu = K.papers[static_cast<std::size_t>(he.u)].field;
        const std::string& fv = K.papers[static_cast<std::size_t>(he.v)].field;
        field_mass[fu] += std::abs(w);
        field_mass[fv] += std::abs(w);
        std::string a = fu;
        std::string b = fv;
        if (a > b) {
            std::swap(a, b);
        }
        pair_mass[a + "|" + b] += std::abs(w);
        if (fu != fv) {
            cross += std::abs(w);
        }
    }
    std::sort(C.support.begin(), C.support.end(), [](const HarmonicEdge& a, const HarmonicEdge& b) {
        return std::abs(a.weight) > std::abs(b.weight);
    });
    C.cross_field_mass = cross;
    C.participation_entropy = shannon(field_mass);
    std::string best_a;
    std::string best_b;
    double best = -1.0;
    for (const auto& kv : pair_mass) {
        if (kv.second > best) {
            best = kv.second;
            const auto bar = kv.first.find('|');
            best_a = kv.first.substr(0, bar);
            best_b = kv.first.substr(bar + 1);
        }
    }
    C.pair_a = best_a;
    C.pair_b = best_b;
    C.paper_loop = trace_loop(K, C.support);
    return C;
}

} // namespace

HarmonicResult extract_harmonic(const SimplicialComplex& K, const HarmonicOptions& opt) {
    HarmonicResult R;
    if (K.n1() == 0) {
        return R;
    }
    const SparseMatrix L1 = hodge_L1(K);
    LanczosOptions lopt;
    lopt.k = std::min(K.n1(), std::max(opt.k, 4));
    lopt.max_steps = opt.lanczos_steps;
    lopt.seed = opt.seed;
    R.lanczos = lanczos_smallest(L1, lopt);
    for (const auto& p : R.lanczos.pairs) {
        R.l1_eigenvalues.push_back(p.value);
    }
    if (!R.l1_eigenvalues.empty()) {
        R.lambda_min = R.l1_eigenvalues.front();
    }
    int betti = 0;
    double gap = 0.0;
    bool saw_pos = false;
    for (double lam : R.l1_eigenvalues) {
        if (lam < opt.harmonic_tol) {
            ++betti;
        } else if (!saw_pos) {
            gap = lam;
            saw_pos = true;
        }
    }
    R.betti1 = betti;
    R.harmonic_gap = gap;

    const int take = std::min(opt.max_cycles, static_cast<int>(R.lanczos.pairs.size()));
    for (int i = 0; i < take; ++i) {
        HarmonicCycle C = cycle_from_vector(K, R.lanczos.pairs[static_cast<std::size_t>(i)], opt.support_frac);
        C.rank = i + 1;
        C.residual = 0.0;
        if (i < R.lanczos.converged) {
            C.residual = R.lanczos.max_residual;
        }
        R.cycles.push_back(std::move(C));
    }
    std::stable_sort(R.cycles.begin(), R.cycles.end(), [](const HarmonicCycle& a, const HarmonicCycle& b) {
        if (std::abs(a.lambda - b.lambda) > 1e-12) {
            return a.lambda < b.lambda;
        }
        if (std::abs(a.cross_field_mass - b.cross_field_mass) > 1e-12) {
            return a.cross_field_mass > b.cross_field_mass;
        }
        return a.participation_entropy > b.participation_entropy;
    });
    for (int i = 0; i < static_cast<int>(R.cycles.size()); ++i) {
        R.cycles[static_cast<std::size_t>(i)].rank = i + 1;
    }

    // Edge-space embedding from the first few L1 eigenvectors (including harmonic).
    const int ek = std::min(opt.k, static_cast<int>(R.lanczos.pairs.size()));
    R.edge_embedding.assign(static_cast<std::size_t>(K.n1()), std::vector<double>(static_cast<std::size_t>(ek), 0.0));
    for (int d = 0; d < ek; ++d) {
        const auto& vec = R.lanczos.pairs[static_cast<std::size_t>(d)].vector;
        for (int e = 0; e < K.n1(); ++e) {
            R.edge_embedding[static_cast<std::size_t>(e)][static_cast<std::size_t>(d)] =
                vec[static_cast<std::size_t>(e)];
        }
    }
    return R;
}

} // namespace hodgeledger
