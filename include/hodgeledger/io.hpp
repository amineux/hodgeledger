#pragma once

#include "hodgeledger/bridges.hpp"
#include "hodgeledger/complex.hpp"
#include "hodgeledger/embedding.hpp"
#include "hodgeledger/harmonic.hpp"
#include "hodgeledger/ledger.hpp"
#include "hodgeledger/timeline.hpp"

#include <string>

namespace hodgeledger {

struct PipelineConfig {
    std::string data_dir = "data/fixtures";
    std::string papers;
    std::string citations;
    std::string categories;
    std::string triangles;
    std::string out_dir = "out";
    std::string docs_dir;
    int k = 8;
    int clusters = 0;
    int harmonic_k = 8;
    int bridges = 24;
    int lanczos_steps = 0;
    unsigned seed = 20260904u;
    bool loo = true;
    int loo_candidates = 32;
    bool timeline = true;
    int max_cycles = 16;
};

struct PipelineResult {
    SimplicialComplex K;
    Embedding embedding;
    HarmonicResult harmonic;
    BridgeReport bridges;
    std::vector<TimelineSlice> slices;
    Ledger ledger;
};

SimplicialComplex load_from_config(const PipelineConfig& cfg);
PipelineResult run_pipeline(const PipelineConfig& cfg);
void export_atlas(const PipelineConfig& cfg, const PipelineResult& R);

} // namespace hodgeledger
