# HodgeLedger

[![ci](https://github.com/amineux/hodgeledger/actions/workflows/ci.yml/badge.svg)](https://github.com/amineux/hodgeledger/actions/workflows/ci.yml)
[![pages](https://github.com/amineux/hodgeledger/actions/workflows/pages.yml/badge.svg)](https://github.com/amineux/hodgeledger/actions/workflows/pages.yml)

**Atlas:** [https://amineux.github.io/hodgeledger/](https://amineux.github.io/hodgeledger/)

Simplicial citation complexes. Hodge Laplacians. Harmonic 1-chains as circular
intellectual debt. A two-chart COBOL ledger.

HodgeLedger is the larger successor to [ClaimLedger](https://github.com/amineux/claimledger):
same DNA (C++20 spectral instrument + GnuCOBOL ledger + static GitHub Pages
atlas), lifted from graphs to **simplicial complexes**.

| ClaimLedger | HodgeLedger |
|-------------|-------------|
| papers + citation edges | papers + edges + **filled triangles** |
| normalized Laplacian \(L_{\mathrm{sym}}\) | Hodge \(L_k = B_k^\top B_k + B_{k+1}B_{k+1}^\top\) |
| Fiedler bridges | **harmonic cycles** (and a Fiedler baseline on the same 0-skeleton) |
| one ledger chart | 0-flow (citations) **and** 1-flow (circulation) |

## Atlas controls

Open the [Pages atlas](https://amineux.github.io/hodgeledger/) (or
`python3 -m http.server --directory docs 8000`).

| input | action |
|-------|--------|
| drag | orbit (3D) / pan (2D) |
| wheel | zoom |
| shift-drag | pan in 3D |
| hover / click | label / select a paper |
| `/` | search |
| `[` `]` | cycle ranked harmonic 1-cycles |
| `2` / `3` | top-down L0 plane / 3D orbit |
| `?` | cheatsheet |
| year slider | dim papers not yet born |

Glowing oriented loops are \(\ker L_1\) — cycles that are not triangle
boundaries.

## Build

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/hodgeledger --data data/fixtures --out out --docs docs/data
python3 scripts/verify_ledger.py --journal out/ledger/journal.csv \
    --dat out/ledger/journal.dat --trial out/ledger/trial_balance.csv
```

CLI: `hodgeledger build | embed | harmonic | timeline | export | run`.

GnuCOBOL is optional (`make -C cobol` or `cmake --build build --target cobol-ledger`).
If `cobc` is missing, the Python verifier is the supported check.

Regenerate synthetic corpora with `python3 scripts/generate_fixtures.py`.
The optional arXiv snapshot is `python3 scripts/fetch_arxiv_slice.py --offline`.

## Math

See [PROBLEM.md](PROBLEM.md) (Hodge Laplacians, harmonic chains, Lanczos
complexity, planted-cycle tests), [SPECS.md](SPECS.md), [ARCHITECTURE.md](ARCHITECTURE.md),
and [data/SCHEMA.md](data/SCHEMA.md).

Triangle rule: **clique complex** of the undirected coupling graph. The
planted cross-field cycle is chordless, hence harmonic.

## Layout

```
include/hodgeledger/  C++20 headers
src/                  complex, Hodge, Lanczos, harmonic, ledger, CLI
tests/                planted-cycle + LOO + conservation
cobol/                POST-FLOW, TRIAL-BALANCE, REPORT
data/fixtures/        synth-NNNN corpus + tiny + optional arxiv-slice
docs/                 static atlas (no bundler)
```

Public repository. No secrets. No telemetry.
