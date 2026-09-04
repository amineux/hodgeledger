#!/usr/bin/env python3
"""Generate HodgeLedger synthetic simplicial corpora.

Triangle rule (clique complex): every 3-clique of the undirected coupling
graph is a 2-simplex. Each field is a stacked chordal complex (H_1 = 0).
Fields are joined by a spanning path (no extra cycles). A chordless C_12
is planted across cs and qbio; those 1-simplices are never filled, so the
cycle is harmonic.

Ids are synth-NNNN only. Seed 20260904.
"""

from __future__ import annotations

import argparse
import csv
import json
import random
from collections import defaultdict
from pathlib import Path

SEED = 20260904

FIELDS = [
    ("cs", "cs.LG", "learning"),
    ("stat", "stat.ML", "statistics"),
    ("math", "math.AT", "topology"),
    ("physics", "physics.comp-ph", "physics"),
    ("qbio", "q-bio.NC", "biology"),
    ("eess", "eess.SP", "signals"),
    ("econ", "econ.TH", "economics"),
    ("qfin", "q-fin.ST", "finance"),
]

TITLES = [
    "Spectral {n} on a {field} complex",
    "Harmonic chains in {field} literature",
    "A stacked {field} note on {n}",
    "Boundary operators for {field} claims",
    "Discrete Hodge theory of {n}",
    "Citation triangles in {field}",
    "On the {field} kernel of L1",
    "Intellectual debt along a {field} cycle",
    "Fiedler versus Hodge on {n}",
    "Simplicial cuts for {field} corpora",
]


def csv_write(path: Path, header: list[str], rows: list[list[object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(header)
        w.writerows(rows)


def stacked_field(nodes: list[int], rng: random.Random) -> tuple[set[tuple[int, int]], list[tuple[int, int, int]]]:
    """Chordal stacked complex: K4, then cone each new vertex onto a live triangle."""
    edges: set[tuple[int, int]] = set()
    triangles: list[tuple[int, int, int]] = []
    if len(nodes) < 4:
        raise ValueError("need at least 4 vertices per field")
    base = nodes[:4]
    for i in range(4):
        for j in range(i + 1, 4):
            a, b = sorted((base[i], base[j]))
            edges.add((a, b))
    live = [
        tuple(sorted((base[0], base[1], base[2]))),
        tuple(sorted((base[0], base[1], base[3]))),
        tuple(sorted((base[0], base[2], base[3]))),
        tuple(sorted((base[1], base[2], base[3]))),
    ]
    triangles.extend(live)
    for v in nodes[4:]:
        a, b, c = live[rng.randrange(len(live))]
        for u in (a, b, c):
            edges.add(tuple(sorted((v, u))))  # type: ignore[arg-type]
        new = [
            tuple(sorted((v, a, b))),
            tuple(sorted((v, b, c))),
            tuple(sorted((v, a, c))),
        ]
        triangles.extend(new)
        live.extend(new)
    return edges, triangles


def make_tiny(out: Path) -> None:
    """12 papers: two filled K4, a path bridge, a dangling unfilled C4.

    Cycle vertices attach to the rest of the complex at *one* vertex so the
    planted C4 is the unique generator of H_1.
    """
    # 1-4 cs K4, 5-8 qbio K4, 9-12 planted C4 (9 cs, 10 qbio, 11 cs, 12 qbio)
    papers = []
    years = {
        1: 2016, 2: 2016, 3: 2016, 4: 2017,
        5: 2016, 6: 2016, 7: 2016, 8: 2017,
        9: 2018, 10: 2018, 11: 2018, 12: 2019,
    }
    fields = {
        1: ("cs", "cs.LG"), 2: ("cs", "cs.LG"), 3: ("cs", "cs.LG"), 4: ("cs", "cs.LG"),
        5: ("qbio", "q-bio.NC"), 6: ("qbio", "q-bio.NC"), 7: ("qbio", "q-bio.NC"), 8: ("qbio", "q-bio.NC"),
        9: ("cs", "cs.LG"), 10: ("qbio", "q-bio.NC"), 11: ("cs", "cs.LG"), 12: ("qbio", "q-bio.NC"),
    }
    for i in range(1, 13):
        field, cat = fields[i]
        papers.append(
            [
                f"synth-{i:04d}",
                f"Tiny {field} paper {i}",
                years[i],
                cat,
                field,
                f"Author {i}",
            ]
        )

    def E(*pairs):
        return {tuple(sorted(p)) for p in pairs}

    edges = E((1, 2), (1, 3), (1, 4), (2, 3), (2, 4), (3, 4))
    edges |= E((5, 6), (5, 7), (5, 8), (6, 7), (6, 8), (7, 8))
    # spanning bridge between the two filled blobs
    edges |= E((4, 5))
    # planted C4, attached by a single stem 1-9 so it is not a boundary
    planted = [(9, 10), (10, 11), (11, 12), (12, 9)]
    edges |= E(*planted)
    edges |= E((1, 9))

    triangles = []
    by = defaultdict(set)
    for a, b in edges:
        by[a].add(b)
        by[b].add(a)
    nodes = list(range(1, 13))
    for i, a in enumerate(nodes):
        for b in nodes[i + 1 :]:
            if b not in by[a]:
                continue
            for c in nodes[nodes.index(b) + 1 :]:
                if c in by[a] and c in by[b]:
                    triangles.append(tuple(sorted((a, b, c))))

    citations = []
    planted_set = {tuple(sorted(p)) for p in planted}
    for a, b in sorted(edges):
        citing, cited = max(a, b), min(a, b)
        y = max(years[citing], years[cited])
        if tuple(sorted((a, b))) in planted_set:
            y = 2019
        citations.append([f"synth-{citing:04d}", f"synth-{cited:04d}", y])

    csv_write(
        out / "papers.csv",
        ["id", "title", "year", "category", "field", "authors"],
        papers,
    )
    csv_write(out / "citations.csv", ["citing", "cited", "year"], citations)
    csv_write(
        out / "triangles.csv",
        ["a", "b", "c"],
        [[f"synth-{a:04d}", f"synth-{b:04d}", f"synth-{c:04d}"] for a, b, c in triangles],
    )
    csv_write(
        out / "categories.csv",
        ["id", "name", "group"],
        [["cs.LG", "Machine Learning", "cs"], ["q-bio.NC", "Neurons and Cognition", "qbio"]],
    )
    planted_json = {
        "cycle_papers": ["synth-0009", "synth-0010", "synth-0011", "synth-0012"],
        "cycle_edges": [
            ["synth-0009", "synth-0010"],
            ["synth-0010", "synth-0011"],
            ["synth-0011", "synth-0012"],
            ["synth-0012", "synth-0009"],
        ],
        "rule": "clique-complex; planted C4 hangs off a single stem and is unfilled",
    }
    (out / "planted.json").write_text(json.dumps(planted_json, indent=2) + "\n", encoding="utf-8")


def make_large(out: Path, per_field: int = 200) -> None:
    rng = random.Random(SEED)
    n_fields = len(FIELDS)
    stacked = per_field
    n_cycle = 12
    n = n_fields * stacked + n_cycle
    papers = []
    field_nodes: list[list[int]] = []
    idx = 0
    for f, (field, cat, _g) in enumerate(FIELDS):
        nodes = list(range(idx, idx + stacked))
        field_nodes.append(nodes)
        idx += stacked
        for local, i in enumerate(nodes):
            year = 2015 + min(9, local // max(1, stacked // 10))
            title = rng.choice(TITLES).format(n=i + 1, field=field)
            authors = f"Synth {field.title()} {1 + (i % 17)}"
            papers.append([f"synth-{i + 1:04d}", title, year, cat, field, authors])

    edges: set[tuple[int, int]] = set()
    triangles: list[tuple[int, int, int]] = []
    for nodes in field_nodes:
        e, t = stacked_field(nodes, rng)
        edges |= e
        triangles.extend(t)

    # Spanning path of blobs (connects the 0-skeleton without extra homology).
    for f in range(n_fields - 1):
        u = field_nodes[f][0]
        v = field_nodes[f + 1][0]
        edges.add(tuple(sorted((u, v))))

    # Dedicated C12 hanging off a single stem on the cs blob — unique H_1.
    cycle_ids = list(range(idx, idx + n_cycle))
    for k, i in enumerate(cycle_ids):
        field, cat, _g = FIELDS[0 if k % 2 == 0 else 4]
        year = 2021 + (k // 6)
        title = rng.choice(TITLES).format(n=i + 1, field=field)
        papers.append([f"synth-{i + 1:04d}", title, year, cat, field, f"Cycle {field.title()} {k}"])
    planted: list[tuple[int, int]] = []
    for k in range(n_cycle):
        planted.append((cycle_ids[k], cycle_ids[(k + 1) % n_cycle]))
    for a, b in planted:
        edges.add(tuple(sorted((a, b))))
    stem = (field_nodes[0][10], cycle_ids[0])
    edges.add(tuple(sorted(stem)))

    years = {i: papers[i][2] for i in range(n)}
    citations = []
    for a, b in sorted(edges):
        citing, cited = (a, b) if years[a] >= years[b] else (b, a)
        if citing == cited:
            citing, cited = max(a, b), min(a, b)
        y = max(years[citing], years[cited])
        key = tuple(sorted((a, b)))
        if key in {tuple(sorted(p)) for p in planted}:
            y = 2022
            papers[citing][2] = max(papers[citing][2], 2021)
            papers[cited][2] = max(int(papers[cited][2]), 2019)
        citations.append([f"synth-{citing + 1:04d}", f"synth-{cited + 1:04d}", y])

    csv_write(
        out / "papers.csv",
        ["id", "title", "year", "category", "field", "authors"],
        papers,
    )
    csv_write(out / "citations.csv", ["citing", "cited", "year"], citations)
    csv_write(
        out / "triangles.csv",
        ["a", "b", "c"],
        [[f"synth-{a + 1:04d}", f"synth-{b + 1:04d}", f"synth-{c + 1:04d}"] for a, b, c in triangles],
    )
    csv_write(
        out / "categories.csv",
        ["id", "name", "group"],
        [[cat, name, field] for field, cat, name in FIELDS],
    )
    planted_json = {
        "cycle_papers": [f"synth-{i + 1:04d}" for i in cycle_ids],
        "cycle_edges": [
            [f"synth-{a + 1:04d}", f"synth-{b + 1:04d}"] for a, b in planted
        ],
        "rule": "clique-complex of stacked chordal fields + spanning path + unfilled C12 on a stem",
        "n_papers": n,
    }
    (out / "planted.json").write_text(json.dumps(planted_json, indent=2) + "\n", encoding="utf-8")
    print(f"large fixture: n={n} edges={len(edges)} triangles={len(triangles)} planted={len(planted)}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="data/fixtures")
    ap.add_argument("--per-field", type=int, default=200)
    args = ap.parse_args()
    root = Path(args.out)
    make_tiny(root / "tiny")
    make_large(root, per_field=args.per_field)
    print("wrote", root)


if __name__ == "__main__":
    main()
