#include "hodgeledger/io.hpp"

#include "hodgeledger/csv.hpp"
#include "hodgeledger/json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace hodgeledger {
namespace fs = std::filesystem;

namespace {

std::string join_data(const PipelineConfig& cfg, const char* name) {
    return (fs::path(cfg.data_dir) / name).string();
}

void write_file(const std::string& path, const std::string& body) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("cannot write " + path);
    }
    out << body;
    if (!body.empty() && body.back() != '\n') {
        out << '\n';
    }
}

void copy_if_docs(const PipelineConfig& cfg, const std::string& src_name) {
    if (cfg.docs_dir.empty()) {
        return;
    }
    fs::create_directories(cfg.docs_dir);
    fs::copy_file(fs::path(cfg.out_dir) / src_name, fs::path(cfg.docs_dir) / src_name,
                  fs::copy_options::overwrite_existing);
}

} // namespace

SimplicialComplex load_from_config(const PipelineConfig& cfg) {
    const std::string papers = cfg.papers.empty() ? join_data(cfg, "papers.csv") : cfg.papers;
    const std::string cites = cfg.citations.empty() ? join_data(cfg, "citations.csv") : cfg.citations;
    std::string cats = cfg.categories.empty() ? join_data(cfg, "categories.csv") : cfg.categories;
    std::string tris = cfg.triangles.empty() ? join_data(cfg, "triangles.csv") : cfg.triangles;
    if (!fs::exists(cats)) {
        cats.clear();
    }
    if (!fs::exists(tris)) {
        tris.clear();
    }
    return load_complex(papers, cites, cats, tris);
}

PipelineResult run_pipeline(const PipelineConfig& cfg) {
    PipelineResult R;
    R.K = load_from_config(cfg);
    R.embedding = embed_complex(R.K, cfg.k, cfg.seed, cfg.lanczos_steps);

    HarmonicOptions hopt;
    hopt.k = cfg.harmonic_k;
    hopt.lanczos_steps = cfg.lanczos_steps;
    hopt.seed = cfg.seed;
    hopt.max_cycles = cfg.max_cycles;
    R.harmonic = extract_harmonic(R.K, hopt);

    BridgeOptions bopt;
    bopt.loo = cfg.loo;
    bopt.loo_candidates = cfg.loo_candidates;
    bopt.top = cfg.bridges;
    bopt.seed = cfg.seed;
    bopt.lanczos_steps = cfg.lanczos_steps;
    if (R.K.n1() <= cfg.loo_candidates) {
        bopt.loo_candidates = R.K.n1();
    }
    R.bridges = score_bridges(R.K, R.harmonic, bopt);

    if (cfg.timeline) {
        R.slices = compute_timeline(R.K, cfg.seed, cfg.lanczos_steps);
    }
    R.ledger = post_ledger(R.K, R.harmonic);
    return R;
}

void export_atlas(const PipelineConfig& cfg, const PipelineResult& R) {
    fs::create_directories(cfg.out_dir);
    fs::create_directories(fs::path(cfg.out_dir) / "ledger");

    const auto& K = R.K;
    const auto& E = R.embedding;
    const auto& H = R.harmonic;
    const auto& B = R.bridges;
    const auto& L = R.ledger;

    {
        JsonWriter j;
        j.begin_object();
        j.key("version");
        j.value(1);
        j.key("n");
        j.value(K.n0());
        j.key("undirected_edges");
        j.value(K.n1());
        j.key("directed_citations");
        j.value(static_cast<int>(K.directed.size()));
        j.key("triangles");
        j.value(K.n2());
        j.key("components");
        j.value(K.component_count());
        j.key("betti0");
        j.value(K.component_count());
        j.key("betti1");
        j.value(H.betti1);
        j.key("algebraic_connectivity_hodge_L0");
        j.value(E.algebraic_connectivity);
        j.key("fiedler_lambda2_normalized");
        j.value(E.fiedler_lambda2);
        j.key("lambda_min_L1");
        j.value(H.lambda_min);
        j.key("harmonic_gap");
        j.value(H.harmonic_gap);
        j.key("loo_candidates");
        j.value(B.loo_candidates);
        j.key("loo_evaluated");
        j.value(B.loo_evaluated);
        j.key("triangle_rule");
        j.value("clique-complex: every 3-clique of the undirected coupling graph is a 2-simplex");
        std::unordered_set<std::string> fields;
        std::unordered_set<std::string> cats;
        for (const auto& p : K.papers) {
            fields.insert(p.field);
            cats.insert(p.category);
        }
        j.key("fields");
        j.begin_array();
        for (const auto& f : fields) {
            j.value(f);
        }
        j.end_array();
        j.key("categories");
        j.begin_array();
        for (const auto& c : cats) {
            j.value(c);
        }
        j.end_array();
        j.key("edges");
        j.begin_array();
        for (const Edge& e : K.edges) {
            j.begin_array();
            j.value(e.u);
            j.value(e.v);
            j.end_array();
        }
        j.end_array();
        j.end_object();
        write_file((fs::path(cfg.out_dir) / "graph_meta.json").string(), j.str());
    }

    {
        JsonWriter j;
        j.begin_object();
        j.key("version");
        j.value(1);
        j.key("k");
        j.value(E.k);
        j.key("algebraic_connectivity");
        j.value(E.algebraic_connectivity);
        j.key("fiedler_lambda2");
        j.value(E.fiedler_lambda2);
        j.key("trivial_eigenvalue");
        j.value(E.trivial_eigenvalue);
        j.key("eigenvalues");
        j.begin_array();
        for (double v : E.l0_eigenvalues) {
            j.value(v);
        }
        j.end_array();
        j.key("l_sym_eigenvalues");
        j.begin_array();
        for (double v : E.l_sym_eigenvalues) {
            j.value(v);
        }
        j.end_array();
        j.key("nodes");
        j.begin_array();
        for (int i = 0; i < K.n0(); ++i) {
            const Paper& p = K.papers[static_cast<std::size_t>(i)];
            const NodeEmbed& nd = E.nodes[static_cast<std::size_t>(i)];
            j.begin_object();
            j.key("id");
            j.value(p.id);
            j.key("title");
            j.value(p.title);
            j.key("year");
            j.value(p.year);
            j.key("category");
            j.value(p.category);
            j.key("field");
            j.value(p.field);
            j.key("authors");
            j.value(p.authors);
            j.key("cluster");
            j.value(nd.cluster);
            j.key("degree");
            j.value(nd.degree);
            const double z = nd.x.size() >= 3 ? nd.x[2] : 0.0;
            j.key("z");
            j.value(z);
            j.key("x");
            j.begin_array();
            for (double v : nd.x) {
                j.value(v);
            }
            j.end_array();
            j.key("u");
            j.begin_array();
            for (double v : nd.u) {
                j.value(v);
            }
            j.end_array();
            j.end_object();
        }
        j.end_array();
        j.end_object();
        write_file((fs::path(cfg.out_dir) / "embedding.json").string(), j.str());
    }

    {
        JsonWriter j;
        j.begin_object();
        j.key("version");
        j.value(1);
        j.key("betti1");
        j.value(H.betti1);
        j.key("lambda_min");
        j.value(H.lambda_min);
        j.key("harmonic_gap");
        j.value(H.harmonic_gap);
        j.key("eigenvalues");
        j.begin_array();
        for (double v : H.l1_eigenvalues) {
            j.value(v);
        }
        j.end_array();
        j.key("cycles");
        j.begin_array();
        for (const auto& C : H.cycles) {
            j.begin_object();
            j.key("rank");
            j.value(C.rank);
            j.key("lambda");
            j.value(C.lambda);
            j.key("energy");
            j.value(C.energy);
            j.key("cross_field_mass");
            j.value(C.cross_field_mass);
            j.key("participation_entropy");
            j.value(C.participation_entropy);
            j.key("pair_a");
            j.value(C.pair_a);
            j.key("pair_b");
            j.value(C.pair_b);
            j.key("harmonic");
            j.value(C.lambda < 1e-4);
            j.key("loop");
            j.begin_array();
            for (const auto& id : C.paper_loop) {
                j.value(id);
            }
            j.end_array();
            j.key("edges");
            j.begin_array();
            for (const auto& e : C.support) {
                j.begin_object();
                j.key("u");
                j.value(K.papers[static_cast<std::size_t>(e.u)].id);
                j.key("v");
                j.value(K.papers[static_cast<std::size_t>(e.v)].id);
                j.key("ui");
                j.value(e.u);
                j.key("vi");
                j.value(e.v);
                j.key("weight");
                j.value(e.weight);
                j.end_object();
            }
            j.end_array();
            j.end_object();
        }
        j.end_array();
        j.end_object();
        write_file((fs::path(cfg.out_dir) / "harmonic.json").string(), j.str());
    }

    {
        JsonWriter j;
        j.begin_object();
        j.key("version");
        j.value(1);
        j.key("method");
        j.value(B.method);
        j.key("prefilter");
        j.value(B.prefilter);
        j.key("loo_candidates");
        j.value(B.loo_candidates);
        j.key("loo_evaluated");
        j.value(B.loo_evaluated);
        j.key("lambda_min_l1");
        j.value(B.lambda_min_l1);
        j.key("bridges");
        j.begin_array();
        for (const auto& b : B.bridges) {
            j.begin_object();
            j.key("id");
            j.value(b.id);
            j.key("rank");
            j.value(b.rank);
            j.key("u");
            j.value(b.u_id);
            j.key("v");
            j.value(b.v_id);
            j.key("score");
            j.value(b.score);
            j.key("abs_flow");
            j.value(b.abs_flow);
            if (b.loo) {
                j.key("delta_lambda");
                j.value(b.delta_lambda);
            } else {
                j.key("delta_lambda");
                j.value(nullptr);
            }
            j.key("loo");
            j.value(b.loo);
            j.key("kind");
            j.value(b.kind);
            j.key("explanation");
            j.value(b.explanation);
            j.end_object();
        }
        j.end_array();
        j.end_object();
        write_file((fs::path(cfg.out_dir) / "bridges.json").string(), j.str());
    }

    {
        JsonWriter j;
        j.begin_object();
        j.key("version");
        j.value(1);
        j.key("slices");
        j.begin_array();
        for (const auto& s : R.slices) {
            j.begin_object();
            j.key("year");
            j.value(s.year);
            j.key("n");
            j.value(s.n);
            j.key("m");
            j.value(s.m);
            j.key("triangles");
            j.value(s.triangles);
            j.key("components");
            j.value(s.components);
            j.key("lambda2");
            j.value(s.lambda2_l0);
            j.key("lambda_min_l1");
            j.value(s.lambda_min_l1);
            j.key("betti1");
            j.value(s.betti1);
            j.key("top_cycle_hint");
            j.value(s.top_cycle_hint);
            j.end_object();
        }
        j.end_array();
        j.end_object();
        write_file((fs::path(cfg.out_dir) / "timeline.json").string(), j.str());
    }

    {
        JsonWriter j;
        j.begin_object();
        j.key("version");
        j.value(1);
        j.key("balanced");
        j.value(L.balanced);
        j.key("total_debit_cents");
        j.value(L.total_debit);
        j.key("total_credit_cents");
        j.value(L.total_credit);
        j.key("total_debit_0");
        j.value(L.total_debit_0);
        j.key("total_credit_0");
        j.value(L.total_credit_0);
        j.key("total_debit_1");
        j.value(L.total_debit_1);
        j.key("total_credit_1");
        j.value(L.total_credit_1);
        j.key("accounts");
        j.begin_array();
        for (const auto& a : L.accounts) {
            j.begin_object();
            j.key("id");
            j.value(a.id);
            j.key("paper_id");
            j.value(a.paper_id);
            j.key("dim");
            j.value(a.dim);
            j.key("debit_cents");
            j.value(a.debit_cents);
            j.key("credit_cents");
            j.value(a.credit_cents);
            j.key("net_cents");
            j.value(a.net_cents);
            j.end_object();
        }
        j.end_array();
        j.key("journal_sample");
        j.begin_array();
        const int nsample = std::min(48, static_cast<int>(L.journal.size()));
        for (int i = 0; i < nsample; ++i) {
            const auto& line = L.journal[static_cast<std::size_t>(i)];
            j.begin_object();
            j.key("je_id");
            j.value(line.je_id);
            j.key("date");
            j.value(line.date);
            j.key("dim");
            j.value(line.dim);
            j.key("debit");
            j.value(line.debit);
            j.key("credit");
            j.value(line.credit);
            j.key("amount_cents");
            j.value(line.amount_cents);
            j.key("memo");
            j.value(line.memo);
            j.end_object();
        }
        j.end_array();
        j.end_object();
        write_file((fs::path(cfg.out_dir) / "ledger.json").string(), j.str());
    }

    write_journal_csv((fs::path(cfg.out_dir) / "ledger" / "journal.csv").string(), L);
    write_journal_dat((fs::path(cfg.out_dir) / "ledger" / "journal.dat").string(), L);
    write_trial_balance_csv((fs::path(cfg.out_dir) / "ledger" / "trial_balance.csv").string(), L);
    write_flows_csv((fs::path(cfg.out_dir) / "ledger" / "flows.csv").string(), L);

    copy_if_docs(cfg, "embedding.json");
    copy_if_docs(cfg, "harmonic.json");
    copy_if_docs(cfg, "bridges.json");
    copy_if_docs(cfg, "graph_meta.json");
    copy_if_docs(cfg, "timeline.json");
    copy_if_docs(cfg, "ledger.json");
}

} // namespace hodgeledger
