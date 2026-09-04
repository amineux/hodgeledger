#include "hodgeledger/timeline.hpp"

#include "hodgeledger/harmonic.hpp"
#include "hodgeledger/hodge.hpp"
#include "hodgeledger/lanczos.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace hodgeledger {

std::vector<TimelineSlice> compute_timeline(const SimplicialComplex& K, unsigned seed,
                                            int lanczos_steps) {
    std::set<int> years;
    for (const auto& p : K.papers) {
        if (p.year > 0) {
            years.insert(p.year);
        }
    }
    for (const auto& d : K.directed) {
        if (d.year > 0) {
            years.insert(d.year);
        }
    }
    std::vector<TimelineSlice> slices;
    slices.reserve(years.size());
    for (int y : years) {
        SimplicialComplex S = K.until_year(y);
        TimelineSlice sl;
        sl.year = y;
        sl.n = S.n0();
        sl.m = S.n1();
        sl.triangles = S.n2();
        sl.components = S.component_count();
        if (S.n0() >= 2) {
            const SparseMatrix L0 = hodge_L0(S);
            LanczosOptions opt;
            opt.k = std::min(S.n0(), std::max(4, S.component_count() + 2));
            opt.max_steps = lanczos_steps > 0 ? std::min(lanczos_steps, 48) : 0;
            opt.seed = seed;
            const auto lr = lanczos_smallest(L0, opt);
            double lam = 0.0;
            bool found = false;
            for (const auto& p : lr.pairs) {
                if (p.value > 1e-8) {
                    lam = p.value;
                    found = true;
                    break;
                }
            }
            sl.lambda2_l0 = found ? lam : 0.0;
        }
        if (S.n1() >= 1) {
            HarmonicOptions hopt;
            hopt.k = std::min(4, S.n1());
            hopt.lanczos_steps = lanczos_steps > 0 ? std::min(lanczos_steps, 48) : 0;
            hopt.seed = seed;
            hopt.max_cycles = 1;
            const HarmonicResult H = extract_harmonic(S, hopt);
            sl.lambda_min_l1 = H.lambda_min;
            sl.betti1 = H.betti1;
            if (!H.cycles.empty() && !H.cycles.front().paper_loop.empty()) {
                sl.top_cycle_hint = H.cycles.front().paper_loop.front();
                if (H.cycles.front().paper_loop.size() > 1) {
                    sl.top_cycle_hint += "→" + H.cycles.front().paper_loop[1];
                }
            }
        }
        slices.push_back(std::move(sl));
    }
    return slices;
}

} // namespace hodgeledger
