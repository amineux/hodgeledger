#include "hodgeledger/io.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void usage() {
    std::cerr
        << "hodgeledger [command] [options]\n"
        << "\n"
        << "Commands:\n"
        << "  run        full pipeline (default): embed + harmonic + LOO + timeline + export\n"
        << "  build      load complex, write graph_meta.json\n"
        << "  embed      Hodge L0 embedding + k-means, write embedding.json\n"
        << "  harmonic   L1 harmonic cycles, write harmonic.json\n"
        << "  timeline   cumulative year slices, write timeline.json\n"
        << "  export     same as run\n"
        << "\n"
        << "Options:\n"
        << "  --data DIR           corpus directory (default data/fixtures)\n"
        << "  --papers PATH        override papers.csv\n"
        << "  --citations PATH     override citations.csv\n"
        << "  --categories PATH    override categories.csv\n"
        << "  --triangles PATH     override triangles.csv\n"
        << "  --out DIR            artifact directory (default out)\n"
        << "  --docs DIR           also copy atlas JSON here (docs/data)\n"
        << "  --k N                L0 embedding dimension (default 8)\n"
        << "  --harmonic-k N       L1 eigenpairs (default 8)\n"
        << "  --clusters N         k-means k (default #fields)\n"
        << "  --bridges N          top simplicial-cut edges (default 24)\n"
        << "  --lanczos-steps N    Krylov dimension (default auto)\n"
        << "  --seed N             RNG seed (default 20260904)\n"
        << "  --loo / --no-loo     leave-one-edge-out Δλ on L1 (default on)\n"
        << "  --loo-candidates N   pre-filter width (default 32)\n"
        << "  --timeline / --no-timeline\n"
        << "  -h, --help\n";
}

bool flag(int& i, int argc, char** argv, const char* a, const char* b = nullptr) {
    if (std::strcmp(argv[i], a) == 0 || (b && std::strcmp(argv[i], b) == 0)) {
        return true;
    }
    return false;
}

std::string need_arg(int& i, int argc, char** argv, const char* name) {
    if (i + 1 >= argc) {
        throw std::runtime_error(std::string("missing argument for ") + name);
    }
    return argv[++i];
}

int need_int(int& i, int argc, char** argv, const char* name) {
    return std::stoi(need_arg(i, argc, argv, name));
}

} // namespace

int main(int argc, char** argv) {
    using namespace hodgeledger;
    PipelineConfig cfg;
    std::string cmd = "run";
    try {
        int start = 1;
        if (argc >= 2 && argv[1][0] != '-') {
            cmd = argv[1];
            start = 2;
        }
        for (int i = start; i < argc; ++i) {
            if (flag(i, argc, argv, "-h", "--help")) {
                usage();
                return 0;
            } else if (flag(i, argc, argv, "--data")) {
                cfg.data_dir = need_arg(i, argc, argv, "--data");
            } else if (flag(i, argc, argv, "--papers")) {
                cfg.papers = need_arg(i, argc, argv, "--papers");
            } else if (flag(i, argc, argv, "--citations")) {
                cfg.citations = need_arg(i, argc, argv, "--citations");
            } else if (flag(i, argc, argv, "--categories")) {
                cfg.categories = need_arg(i, argc, argv, "--categories");
            } else if (flag(i, argc, argv, "--triangles")) {
                cfg.triangles = need_arg(i, argc, argv, "--triangles");
            } else if (flag(i, argc, argv, "--out")) {
                cfg.out_dir = need_arg(i, argc, argv, "--out");
            } else if (flag(i, argc, argv, "--docs")) {
                cfg.docs_dir = need_arg(i, argc, argv, "--docs");
            } else if (flag(i, argc, argv, "--k")) {
                cfg.k = need_int(i, argc, argv, "--k");
            } else if (flag(i, argc, argv, "--harmonic-k")) {
                cfg.harmonic_k = need_int(i, argc, argv, "--harmonic-k");
            } else if (flag(i, argc, argv, "--clusters")) {
                cfg.clusters = need_int(i, argc, argv, "--clusters");
            } else if (flag(i, argc, argv, "--bridges")) {
                cfg.bridges = need_int(i, argc, argv, "--bridges");
            } else if (flag(i, argc, argv, "--lanczos-steps")) {
                cfg.lanczos_steps = need_int(i, argc, argv, "--lanczos-steps");
            } else if (flag(i, argc, argv, "--seed")) {
                cfg.seed = static_cast<unsigned>(need_int(i, argc, argv, "--seed"));
            } else if (flag(i, argc, argv, "--loo")) {
                cfg.loo = true;
            } else if (flag(i, argc, argv, "--no-loo")) {
                cfg.loo = false;
            } else if (flag(i, argc, argv, "--loo-candidates")) {
                cfg.loo_candidates = need_int(i, argc, argv, "--loo-candidates");
            } else if (flag(i, argc, argv, "--timeline")) {
                cfg.timeline = true;
            } else if (flag(i, argc, argv, "--no-timeline")) {
                cfg.timeline = false;
            } else {
                throw std::runtime_error(std::string("unknown option: ") + argv[i]);
            }
        }

        if (cmd == "build") {
            auto K = load_from_config(cfg);
            PipelineResult R;
            R.K = std::move(K);
            export_atlas(cfg, R);
            std::cerr << "build: n=" << R.K.n0() << " m=" << R.K.n1() << " triangles=" << R.K.n2()
                      << "\n";
            return 0;
        }

        if (cmd != "run" && cmd != "export" && cmd != "embed" && cmd != "harmonic" &&
            cmd != "timeline") {
            throw std::runtime_error("unknown command: " + cmd);
        }

        if (cmd == "embed") {
            cfg.loo = false;
            cfg.timeline = false;
        } else if (cmd == "harmonic") {
            cfg.loo = false;
            cfg.timeline = false;
        } else if (cmd == "timeline") {
            cfg.loo = false;
        }

        const PipelineResult R = run_pipeline(cfg);
        export_atlas(cfg, R);
        std::cerr << "hodgeledger " << cmd << ": n=" << R.K.n0() << " m=" << R.K.n1()
                  << " triangles=" << R.K.n2() << " β0=" << R.K.component_count()
                  << " β1~=" << R.harmonic.betti1 << " λ2(L0)=" << R.embedding.algebraic_connectivity
                  << " λ2(Fiedler)=" << R.embedding.fiedler_lambda2
                  << " λmin(L1)=" << R.harmonic.lambda_min << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "hodgeledger: " << ex.what() << "\n";
        return 1;
    }
}
