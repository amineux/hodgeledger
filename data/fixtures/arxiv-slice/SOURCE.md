# arXiv slice source

Committed public snapshot from the arXiv Atom API
(`https://export.arxiv.org/api/query`), 36 recent records spanning
`cs.LG` / `stat.ML` / `math.AT` (fetched 2026-09-04). Rebuild CSVs without
the network:

```
python3 scripts/fetch_arxiv_slice.py --out data/fixtures/arxiv-slice --offline
```

- Edge rule: **coupling**, not citations. An undirected edge exists when two
  records share an author or a category (including cross-lists).
- Triangle rule: clique complex of that coupling graph.
- Famous papers that happen to appear are **not** given forged citation edges.
- The HodgeLedger CLI never fetches. `--offline` is what CI runs.
