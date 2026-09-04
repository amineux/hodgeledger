# HodgeLedger specification

## Product

HodgeLedger is a research instrument that treats the scientific literature as
a **simplicial complex** and as a **two-chart ledger**:

1. a **Hodge system** — boundary operators \(B_1, B_2\), Hodge Laplacians
   \(L_0, L_1\) (and \(L_2\) on small complexes), Lanczos spectra, harmonic
   1-chains, L0/L1 embeddings, and leave-one-edge-out simplicial cuts;
2. an **accounting system** — chart 0 posts citation (0-flow) debt, chart 1
   posts oriented harmonic circulation (1-flow) along \(\ker L_1\).

The instrument is a C++20 CLI, a GnuCOBOL ledger (with a Python verifier),
and a static atlas under `docs/`.

## ClaimLedger vs HodgeLedger

| | ClaimLedger | HodgeLedger |
|--|-------------|-------------|
| Geometry | undirected graph | clique complex (0-, 1-, 2-simplices) |
| Operator | normalized Laplacian \(L_{\mathrm{sym}}\) | Hodge \(L_k = B_k^\top B_k + B_{k+1}B_{k+1}^\top\) |
| Kernel story | Fiedler cut / algebraic connectivity | harmonic 1-chains = circular intellectual debt |
| Embedding | vertex eigenmaps of \(L_{\mathrm{sym}}\) | vertex eigenmaps of \(L_0\) **and** edge eigenmaps of \(L_1\) |
| Interdiction | leave-one-**vertex**-out Δλ₂ | leave-one-**edge**-out Δλ_min(\(L_1\)) |
| Ledger | one chart of paper accounts | chart 0 (0-flow) and chart 1 (1-flow) |
| Atlas | bridges as amber points | **glowing oriented harmonic loops** + 2D/3D orbit |

HodgeLedger still computes ClaimLedger’s Fiedler \(\lambda_2(L_{\mathrm{sym}})\)
on the same 0-skeleton and writes it next to Hodge \(\lambda_2(L_0)\) so the
graph-only baseline is comparable, not replaced in silence.

## Non-goals

- Not a crawler. The CLI never hits the network.
- Not an FPTAS for combinatorial homology or for \(k>1\) simplicial interdiction.
- Not a replacement for OpenAlex / Semantic Scholar.
- No secrets, no accounts, no network I/O in the CLI.

## CLI

```
hodgeledger [command] [options]
```

| command | effect |
|---------|--------|
| `run` | Full pipeline (default): embed + harmonic + LOO + timeline + export. |
| `build` | Load complex, write `graph_meta.json`. |
| `embed` | \(L_0\) Lanczos + k-means, write `embedding.json`. |
| `harmonic` | \(L_1\) harmonic extraction, write `harmonic.json`. |
| `timeline` | Cumulative year snapshots, write `timeline.json`. |
| `export` | Same as `run`. |

| option | default | meaning |
|--------|---------|---------|
| `--data DIR` | `data/fixtures` | papers/citations/triangles CSVs |
| `--out DIR` | `out` | artifacts |
| `--docs DIR` | *(off)* | also copy atlas JSON to `docs/data` |
| `--k N` | `8` | nontrivial \(L_0\) embedding dimension |
| `--harmonic-k N` | `8` | smallest \(L_1\) eigenpairs |
| `--bridges N` | `24` | top simplicial-cut edges emitted |
| `--lanczos-steps N` | auto | Krylov dimension |
| `--seed N` | `20260904` | Lanczos + k-means |
| `--loo` / `--no-loo` | on | leave-one-edge-out Δλ on the shortlist |
| `--loo-candidates N` | `32` | pre-filter width; tiny fixture evaluates every edge |
| `--timeline` / `--no-timeline` | on | cumulative year slices |

Exit status is `0` on success, `1` on user/data/solver errors. Diagnostics
go to stderr.

## Complex construction

- 0-simplices = papers, file order.
- Directed citations stored for 0-flow. Undirected 1-simplices oriented
  \(u \to v\) with \(u < v\).
- 2-simplices = 3-cliques of the undirected graph (clique complex). Isolated
  papers are kept.

See `PROBLEM.md` for \(B_k\), \(L_k\), and the Lanczos-on-\(L_1\) heuristic.

## Spectral pipeline

1. Assemble sparse \(B_1, B_2\) in CSR.
2. \(L_0 = B_1 B_1^\top\), \(L_1 = B_1^\top B_1 + B_2 B_2^\top\).
3. Lanczos with full double reorthogonalization, Jacobi Ritz, multiplicity restart.
4. Drop near-zero \(L_0\) modes (one per connected component); remaining
   eigenvectors are the atlas coordinates. Sign: largest-magnitude entry > 0.
5. Row-normalize for k-means++ (Ng–Jordan–Weiss), \(k =\) #fields.
6. Smallest \(L_1\) eigenpairs: \(\lambda \approx 0\) are harmonic 1-chains.
   Support-threshold the cochain, trace a loop, rank by \(\lambda\) then
   cross-field mass then participation entropy.
7. Pre-filter edges by \(|f_e|\) of the leading harmonic mode; leave-one-edge-out
   rebuilds \(L_1\) and reports \(\Delta\lambda_{\min}\).
8. Also solve \(L_{\mathrm{sym}}\) of the 0-skeleton for the ClaimLedger Fiedler baseline.

Residuals \(\|Lq-\lambda q\|\) are diagnostic.

### Timeline

For each distinct paper year \(t\), restrict to papers with `year <= t` and
to edges/triangles whose members exist in that slice. Record
\(n, m, n_2, \beta_0, \lambda_2(L_0), \lambda_{\min}(L_1), \tilde\beta_1\).

## Ledger

**Chart 0 (0-flow).** Each directed citation:

```
DR  0:<citing>     100   intellectual debt
CR  0:<cited>      100   intellectual capital
```

**Chart 1 (1-flow).** For the leading harmonic 1-cochain \(f\), each support
edge with signed weight \(f_e\) posts, in the direction of the flow,
an amount \(\mathrm{round}(100\,|f_e|/\max|f|)\):

```
DR  1:<src>        amt   harmonic circulation
CR  1:<dst>        amt
```

A harmonic cochain is divergence-free (\(B_1 f = 0\)), so chart 1 nets near
zero at every vertex — conservation that graph ledgers cannot even state.

Date is synthetic mid-year `YYYY0615`. Invariants: distinct nonempty accounts,
positive amounts, \(\sum\mathrm{DR}=\sum\mathrm{CR}\) globally and per chart.
Re-posting is deterministic.

COBOL programs `POST-FLOW`, `TRIAL-BALANCE`, `REPORT` are a second
implementation. `POST-FLOW` sets `COB_LS_FIXED` so GnuCOBOL LINE SEQUENTIAL
keeps the canonical 96-byte `journal.dat` records (otherwise trailing memo
spaces are stripped). If `cobc` is missing, `scripts/verify_ledger.py` is the
supported check.

## Atlas

`docs/index.html` is Pages-ready: no bundler. It must render from checked-in
`docs/data/*.json`.

Controls: drag orbit (3D) / pan (2D), wheel zoom, hover labels, click select,
`/` search, `[` `]` cycle harmonic loops, `2`/`3` view mode, `?` cheatsheet,
year slider dims unborn papers. Harmonic 1-cycles are glowing oriented edge
loops — the star of the show.

## Build / test

```
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/hodgeledger --data data/fixtures --out out --docs docs/data
python3 scripts/verify_ledger.py --journal out/ledger/journal.csv
```

C++20, `-Wall -Wextra -Wpedantic`. GoogleTest via FetchContent. Tests cover
CSR, \(B_1 B_2 = 0\), C₆ / K₅ spectra, planted-cycle recovery, leave-one-edge-out
Δλ, journal conservation, cumulative slices.

## Reproducibility

Same corpus + same `--seed --k --lanczos-steps` ⇒ same eigenvalues up to
floating-point noise, same planted-cycle ranking on the tiny fixture,
bit-identical journal CSV.

## License / security

Public repository, no credentials, no telemetry. Fixture text is original
synthetic prose. The arXiv slice is a committed public Atom snapshot.
