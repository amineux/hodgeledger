# Architecture

HodgeLedger is three programs that share one journal and one pair of Hodge
Laplacians.

```
 papers.csv  citations.csv  triangles.csv
            │
            ▼
   ┌────────────────────────────┐
   │  C++20 core                │  clique complex → B1, B2 (CSR)
   │  libhodgeledger            │  L0, L1 → Lanczos → embed / harmonic
   │                            │  leave-one-edge-out Δλ_min(L1)
   │                            │  cumulative year slices + two-chart poster
   └────────┬───────────────────┘
            │
            ├─ out/embedding.json
            ├─ out/harmonic.json
            ├─ out/bridges.json
            ├─ out/timeline.json
            ├─ out/graph_meta.json
            ├─ out/ledger.json
            └─ out/ledger/{journal.dat,journal.csv,flows.csv,trial_balance.csv}
                    │
                    ▼
        ┌───────────────────────┐         ┌─────────────────────────┐
        │  COBOL (cobc)         │         │  Python verifier        │
        │  POST-FLOW            │         │  scripts/verify_ledger  │
        │  TRIAL-BALANCE        │         │  (always available)     │
        │  REPORT               │         └─────────────────────────┘
        └───────────────────────┘
                    │
                    ▼
              docs/data/*.json  ──►  docs/index.html  (GitHub Pages)
```

## Components

### `include/hodgeledger/` + `src/`

| unit | responsibility |
|------|----------------|
| `complex` | papers + directed cites → oriented 1-skeleton + clique triangles; CSR \(B_1,B_2\) |
| `sparse` | rectangular CSR, \(AA^\top\), \(A^\top A\), matvec |
| `hodge` | \(L_0, L_1, L_2\) and ClaimLedger \(L_{\mathrm{sym}}\) on the 0-skeleton |
| `lanczos` | smallest eigenpairs, full reorth. + Jacobi Ritz + multiplicity restart |
| `embedding` | drop \(\ker L_0\), export raw + row-normalized coords, k-means++ |
| `harmonic` | approx \(\ker L_1\), cycle trace, field participation ranking |
| `bridges` | harmonic-mass pre-filter + leave-one-edge-out \(\Delta\lambda_{\min}(L_1)\) |
| `timeline` | cumulative year snapshots of \(n,m,n_2,\lambda_2,\tilde\beta_1\) |
| `ledger` | 0-flow citations + 1-flow harmonic circulation; 96-byte `journal.dat` |
| `csv` / `json` | zero-dependency interchange |
| `io` | corpus load and atlas writers |
| `main` | CLI: `run` / `build` / `embed` / `harmonic` / `timeline` / `export` |

The library is a static target `hodgeledger_core`. The CLI links it. Tests
link it plus GoogleTest (FetchContent, v1.14.0).

### COBOL (`cobol/`)

GnuCOBOL free-format programs, compiled with `cobc -free -I cobol/copy`.
`IDENTIFICATION DIVISION` contains only `PROGRAM-ID` — prose comments there
break `cobc`. COPY books `JOURNAL.cpy` and `ACCOUNT.cpy` are the contract
with C++ `write_journal_dat()`. Record length is 96 bytes.

`TRIAL-BALANCE` holds up to 8000 in-memory accounts (two charts × papers).
The Python verifier has no such ceiling and is what CI runs when `cobc` is
missing.

### Atlas (`docs/`)

Vanilla HTML / CSS / Canvas. No bundler, no npm. GitHub Pages serves `/docs`.
The page fetches `docs/data/*.json` and draws:

- paper points from the first three nontrivial \(L_0\) coordinates
- **harmonic 1-cycles as glowing oriented edge loops**
- optional translucent triangle fills on the selected cycle’s support
- field colours, 2D Fiedler-style plane ↔ 3D orbit, search, year slider,
  COBOL-terminal ledger for the selected paper *and* the selected cycle

Keyboard: `/`, `[` `]`, `2`/`3`, `?`, wheel zoom, drag orbit/pan.

### CI (`.github/workflows/`)

- `ci.yml` — configure, build, `ctest`, CLI on the tiny fixture, verifier,
  optional `cobc`.
- `pages.yml` — deploy `docs/` to GitHub Pages.

## Build graph

```
CMakeLists.txt
 ├─ hodgeledger_core (static)
 │    src/{sparse,csv,json,complex,hodge,lanczos,embedding,
 │         harmonic,bridges,timeline,ledger,io}.cpp
 ├─ hodgeledger           → src/main.cpp + core
 ├─ hodgeledger_tests     → tests/*.cpp + core + GTest
 └─ cobol-ledger          → cobc (optional)
```

```
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/hodgeledger --data data/fixtures --out out --docs docs/data
python3 scripts/verify_ledger.py --journal out/ledger/journal.csv \
    --dat out/ledger/journal.dat --trial out/ledger/trial_balance.csv
```

## Data-flow invariants

1. \(B_1 B_2 = 0\) (simplicial identity). Checked in tests.
2. \(L_0 = D - A\) on the 0-skeleton.
3. Every journal line is a balanced pair. Charts 0 and 1 each balance.
4. Atlas JSON is a pure function of the corpus + solver flags.
5. Default fixture ids match `synth-NNNN`. The optional arXiv slice uses
   real public ids; its edges are coupling, not citations.

## Why COBOL

The ledger is the same information as the complex, restated in the oldest
language still used to move money. A citation is a 0-flow transfer. A
harmonic 1-chain is a circulation: debt that returns to its source. A trial
balance that does not zero is a bug in science *or* in the parser. Putting
that check in GnuCOBOL is a type system for conservation laws that graph
libraries usually skip — now in two dimensions.
