# Data schema

HodgeLedger reads UTF-8 CSV (quoted fields, doubled quotes) and never invents
missing identifiers. Dangling citation ids and self-loops are skipped.

## papers.csv

| column | type | notes |
|--------|------|--------|
| `id` | string | Synthetic corpora use `synth-NNNN` only. arXiv slice uses public arXiv ids. |
| `title` | string | Original synthetic prose, or the public Atom title. |
| `year` | int | Publication year. |
| `category` | string | arXiv-style category (`cs.LG`, …). |
| `field` | string | Coarse field used for colour and participation (`cs`, `qbio`, …). |
| `authors` | string | Free text. |

Row order is the 0-simplex order: vertex index `0..n-1`.

## citations.csv

| column | type | notes |
|--------|------|--------|
| `citing` | string | Paper id. |
| `cited` | string | Paper id. |
| `year` | int | Observation year (citing year). |
| `kind` | string | Optional. Default `citation`. The arXiv slice uses `shared-author` / `shared-category` **coupling**. |

Each surviving directed row is stored for the 0-flow ledger. The undirected
1-skeleton is the symmetric closure (self-loops dropped). Multiplicity is not
kept: the complex is simple.

## triangles.csv

| column | type | notes |
|--------|------|--------|
| `a`,`b`,`c` | string | Three paper ids of a filled 2-simplex. Order does not matter; the loader sorts them. |

### Triangle rule (clique complex)

A triple is a 2-simplex **if and only if** all three pairwise undirected
coupling/citation edges exist. That is the flag / clique complex of the
0-skeleton. The generator writes the filled list; if `triangles.csv` is
absent the C++ loader infers the same cliques.

The planted harmonic cycle is **chordless**: it is a 1-cycle that is not a
boundary of any 2-chain, so it lives in \(\ker L_1\).

## categories.csv

| column | type | notes |
|--------|------|--------|
| `id` | string | Category id. |
| `name` | string | Display name. |
| `group` | string | Field group. |

Optional.

## planted.json (fixtures only)

Documents the synthetic harmonic 1-cycle for unit tests: `cycle_papers`,
`cycle_edges`. Not consumed by the CLI.

## Journal (C++ ↔ COBOL)

`out/ledger/journal.csv` — `je_id,date,dim,debit_account,credit_account,amount_cents,memo`.

`out/ledger/flows.csv` — `dim,debit,credit,year,amount_cents,memo` (COBOL `POST-FLOW` input).

`out/ledger/journal.dat` — LINE SEQUENTIAL, **96 bytes + newline**.
GnuCOBOL LINE SEQUENTIAL strips trailing spaces unless `COB_LS_FIXED` is
true; `POST-FLOW` sets that environment at start so the DAT file stays 96
bytes (matching C++ `write_journal_dat()`).

Layout:

```
01 JOURNAL-REC.
   05 JE-ID         PIC 9(8).
   05 JE-DATE       PIC 9(8).
   05 JE-DIM        PIC 9.
   05 JE-FILL       PIC X.
   05 DEBIT-ACCT    PIC X(16).
   05 CREDIT-ACCT   PIC X(16).
   05 AMOUNT-CENTS  PIC 9(10).
   05 MEMO          PIC X(36).
```

Account ids are chart-prefixed: `0:synth-0001` (citation / 0-flow) and
`1:synth-0005` (harmonic circulation / 1-flow). Truncated to 16 characters
in the DAT file.

`out/ledger/trial_balance.csv` — `dim,account,debit_cents,credit_cents,net_cents`
plus a `TOTAL` row. `net = credit - debit`.

## Atlas JSON (`out/` and `docs/data/`)

Pretty-printed, `version` 1.

| file | contents |
|------|----------|
| `embedding.json` | L0 Hodge embedding, k-means cluster, raw `x` and unit `u`, `z`. |
| `harmonic.json` | L1 spectrum, ranked 1-cycles with oriented edge weights and traced loops. |
| `bridges.json` | Leave-one-edge-out Δλ_min(L₁) on a harmonic-mass shortlist. |
| `graph_meta.json` | n, m, triangles, β₀, β₁, Fiedler λ₂ vs Hodge λ₂, integer edge list. |
| `timeline.json` | Cumulative year slices. |
| `ledger.json` | Both charts, accounts, journal sample. |

## Fixtures

- `data/fixtures/tiny/` — 12 papers, two filled K₄, planted unfilled C₄ on a stem, path bridge.
- `data/fixtures/` — ≥1500-paper stacked chordal fields + spanning path + planted C₁₂.
- `data/fixtures/arxiv-slice/` — optional public Atom snapshot; coupling edges only.
