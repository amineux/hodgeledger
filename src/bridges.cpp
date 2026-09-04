#include "hodgeledger/bridges.hpp"

#include "hodgeledger/hodge.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace hodgeledger {

BridgeReport score_bridges(const SimplicialComplex& K, const HarmonicResult& harm,
                           const BridgeOptions& opt) {
    BridgeReport R;
    R.prefilter = "harmonic-flow-magnitude";
    R.lambda_min_l1 = harm.lambda_min;
    if (K.n1() == 0) {
        R.method = "none";
        return R;
    }

    std::vector<double> flow(static_cast<std::size_t>(K.n1()), 0.0);
    if (!harm.cycles.empty()) {
        const auto& v = harm.cycles.front().vector;
        for (int e = 0; e < K.n1() && e < static_cast<int>(v.size()); ++e) {
            flow[static_cast<std::size_t>(e)] = std::abs(v[static_cast<std::size_t>(e)]);
        }
    }

    std::vector<int> order(static_cast<std::size_t>(K.n1()));
    for (int e = 0; e < K.n1(); ++e) {
        order[static_cast<std::size_t>(e)] = e;
    }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (flow[static_cast<std::size_t>(a)] != flow[static_cast<std::size_t>(b)]) {
            return flow[static_cast<std::size_t>(a)] > flow[static_cast<std::size_t>(b)];
        }
        return a < b;
    });

    const int shortlist = opt.loo ? std::min(K.n1(), std::max(opt.loo_candidates, 1)) : 0;
    R.loo_candidates = shortlist;

    std::vector<Bridge> cand;
    cand.reserve(static_cast<std::size_t>(K.n1()));
    for (int e = 0; e < K.n1(); ++e) {
        Bridge b;
        b.edge = e;
        b.u_id = K.papers[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].u)].id;
        b.v_id = K.papers[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].v)].id;
        b.id = b.u_id + "|" + b.v_id;
        b.abs_flow = flow[static_cast<std::size_t>(e)];
        b.score = b.abs_flow;
        b.kind = "edge";
        const std::string& fu = K.papers[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].u)].field;
        const std::string& fv = K.papers[static_cast<std::size_t>(K.edges[static_cast<std::size_t>(e)].v)].field;
        std::ostringstream ex;
        ex << "1-cochain mass " << b.abs_flow;
        if (fu != fv) {
            ex << " on a " << fu << "–" << fv << " cut edge";
        } else {
            ex << " inside field " << fu;
        }
        b.explanation = ex.str();
        cand.push_back(std::move(b));
    }
    std::stable_sort(cand.begin(), cand.end(), [](const Bridge& a, const Bridge& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.id < b.id;
    });

    if (opt.loo && shortlist > 0) {
        R.method = "leave-one-edge-out";
        LanczosOptions lopt;
        lopt.k = std::min(4, std::max(1, K.n1() - 1));
        lopt.max_steps = opt.lanczos_steps;
        lopt.seed = opt.seed;
        int evaluated = 0;
        for (int i = 0; i < static_cast<int>(cand.size()) && evaluated < shortlist; ++i) {
            const int e = cand[static_cast<std::size_t>(i)].edge;
            SimplicialComplex K2 = K.without_edge(e);
            if (K2.n1() == 0) {
                cand[static_cast<std::size_t>(i)].delta_lambda = harm.lambda_min;
                cand[static_cast<std::size_t>(i)].loo = true;
                ++evaluated;
                continue;
            }
            const SparseMatrix L1 = hodge_L1(K2);
            lopt.k = std::min(lopt.k, K2.n1());
            const LanczosResult lr = lanczos_smallest(L1, lopt);
            const double lam = lr.pairs.empty() ? 0.0 : lr.pairs.front().value;
            cand[static_cast<std::size_t>(i)].delta_lambda = lam - harm.lambda_min;
            cand[static_cast<std::size_t>(i)].loo = true;
            ++evaluated;
        }
        R.loo_evaluated = evaluated;
        std::stable_sort(cand.begin(), cand.end(), [](const Bridge& a, const Bridge& b) {
            if (a.loo != b.loo) {
                return a.loo;
            }
            if (a.loo && b.loo && std::abs(a.delta_lambda - b.delta_lambda) > 1e-12) {
                return a.delta_lambda > b.delta_lambda;
            }
            if (a.score != b.score) {
                return a.score > b.score;
            }
            return a.id < b.id;
        });
    } else {
        R.method = "harmonic-flow";
    }

    const int top = std::min(opt.top, static_cast<int>(cand.size()));
    for (int i = 0; i < top; ++i) {
        cand[static_cast<std::size_t>(i)].rank = i + 1;
        R.bridges.push_back(cand[static_cast<std::size_t>(i)]);
    }
    return R;
}

} // namespace hodgeledger
