#!/usr/bin/env python3
"""Optional arXiv Atom snapshot for HodgeLedger.

The CLI never hits the network. This script can refresh
data/fixtures/arxiv-slice/ from the public arXiv API, or rebuild CSVs
from the committed atom.xml in --offline mode.

Edges are author / cross-list *coupling*, never forged citation links
between famous papers. 2-simplices are 3-cliques of that coupling graph.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import sys
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

ATOM = "{http://www.w3.org/2005/Atom}"
ARXIV = "{http://arxiv.org/schemas/atom}"
DEFAULT_QUERY = "cat:cs.LG+OR+cat:stat.ML+OR+cat:math.AT"
UA = "hodgeledger/0.1 (https://github.com/amineux/hodgeledger; research snapshot)"


def parse_atom(text: str) -> list[dict]:
    root = ET.fromstring(text)
    papers = []
    for entry in root.findall(f"{ATOM}entry"):
        eid = (entry.findtext(f"{ATOM}id") or "").strip()
        arxiv_id = eid.rsplit("/", 1)[-1]
        arxiv_id = re.sub(r"v\d+$", "", arxiv_id)
        title = re.sub(r"\s+", " ", (entry.findtext(f"{ATOM}title") or "").strip())
        published = (entry.findtext(f"{ATOM}published") or "")[:4]
        authors = []
        for a in entry.findall(f"{ATOM}author"):
            name = (a.findtext(f"{ATOM}name") or "").strip()
            if name:
                authors.append(name)
        cats = [c.attrib.get("term", "") for c in entry.findall(f"{ATOM}category")]
        cats = [c for c in cats if c]
        primary = cats[0] if cats else "cs.LG"
        field = primary.split(".")[0] if primary else "cs"
        if field == "q-bio":
            field = "qbio"
        papers.append(
            {
                "id": arxiv_id,
                "title": title,
                "year": int(published) if published.isdigit() else 2017,
                "category": primary,
                "field": field,
                "authors": "; ".join(authors),
                "categories": cats,
                "author_list": authors,
            }
        )
    return papers


def coupling_edges(papers: list[dict]) -> list[tuple[str, str, int, str]]:
    """Shared-author or shared-category (cross-list) coupling. Not citations."""
    edges = []
    seen = set()
    for i, a in enumerate(papers):
        aset = {x.lower() for x in a["author_list"]}
        acats = set(a["categories"])
        for b in papers[i + 1 :]:
            bset = {x.lower() for x in b["author_list"]}
            bcats = set(b["categories"])
            reason = None
            if aset & bset:
                reason = "shared-author"
            elif acats & bcats:
                reason = "shared-category"
            if not reason:
                continue
            u, v = sorted((a["id"], b["id"]))
            if (u, v) in seen:
                continue
            seen.add((u, v))
            year = max(a["year"], b["year"])
            edges.append((a["id"] if a["id"] == v or a["year"] >= b["year"] else b["id"], u if a["id"] == v else (v if a["id"] == u else b["id"]), year, reason))
            # citing, cited as newer → older coupling observation
    # rebuild cleanly
    out = []
    for i, a in enumerate(papers):
        aset = {x.lower() for x in a["author_list"]}
        acats = set(a["categories"])
        for b in papers[i + 1 :]:
            bset = {x.lower() for x in b["author_list"]}
            bcats = set(b["categories"])
            reason = None
            if aset & bset:
                reason = "shared-author"
            elif len(acats & bcats) >= 1 and (len(acats) > 1 or len(bcats) > 1 or acats == bcats):
                # same primary category is still coupling, labelled as such
                reason = "shared-category"
            if not reason:
                continue
            newer, older = (a, b) if a["year"] >= b["year"] else (b, a)
            out.append((newer["id"], older["id"], newer["year"], reason))
    return out


def triangles_of(papers: list[dict], edges: list[tuple[str, str, int, str]]) -> list[tuple[str, str, str]]:
    ids = [p["id"] for p in papers]
    adj = defaultdict(set)
    for citing, cited, _y, _r in edges:
        adj[citing].add(cited)
        adj[cited].add(citing)
    tris = []
    for i, a in enumerate(ids):
        for b in ids[i + 1 :]:
            if b not in adj[a]:
                continue
            for c in ids[ids.index(b) + 1 :]:
                if c in adj[a] and c in adj[b]:
                    tris.append(tuple(sorted((a, b, c))))
    return tris


def write_slice(out: Path, papers: list[dict], atom_text: str | None, source: str) -> None:
    out.mkdir(parents=True, exist_ok=True)
    if atom_text is not None:
        (out / "atom.xml").write_text(atom_text, encoding="utf-8")
    with (out / "papers.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["id", "title", "year", "category", "field", "authors"])
        for p in papers:
            w.writerow([p["id"], p["title"], p["year"], p["category"], p["field"], p["authors"]])
    edges = coupling_edges(papers)
    with (out / "citations.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["citing", "cited", "year", "kind"])
        for row in edges:
            w.writerow(list(row))
    tris = triangles_of(papers, edges)
    with (out / "triangles.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["a", "b", "c"])
        w.writerows(tris)
    cats = {}
    for p in papers:
        cats[p["category"]] = p["field"]
    with (out / "categories.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["id", "name", "group"])
        for cid, grp in sorted(cats.items()):
            w.writerow([cid, cid, grp])
    (out / "SOURCE.md").write_text(
        f"""# arXiv slice source

- Query / origin: {source}
- Records: {len(papers)}
- Edge rule: **coupling**, not citations. An undirected edge exists when two
  records share an author or a category (including cross-lists).
- Triangle rule: clique complex of that coupling graph.
- Famous papers that happen to appear are **not** given forged citation edges.
- The CLI never fetches; `--offline` rebuilds CSVs from committed `atom.xml`.
""",
        encoding="utf-8",
    )
    print(f"arxiv-slice: n={len(papers)} coupling={len(edges)} triangles={len(tris)}")


def fetch_atom(query: str, max_results: int) -> str:
    q = urllib.parse.quote(query, safe=":+")
    url = (
        "https://export.arxiv.org/api/query?"
        f"search_query={q}&start=0&max_results={max_results}&sortBy=submittedDate&sortOrder=descending"
    )
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=60) as resp:
        return resp.read().decode("utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="data/fixtures/arxiv-slice")
    ap.add_argument("--offline", action="store_true")
    ap.add_argument("--query", default=DEFAULT_QUERY)
    ap.add_argument("--max-results", type=int, default=40)
    args = ap.parse_args()
    out = Path(args.out)
    atom_path = out / "atom.xml"
    if args.offline:
        if not atom_path.exists():
            print("offline snapshot missing:", atom_path, file=sys.stderr)
            return 1
        text = atom_path.read_text(encoding="utf-8")
        papers = parse_atom(text)
        write_slice(out, papers, None, f"offline atom.xml sha256={hashlib.sha256(text.encode()).hexdigest()[:12]}")
        return 0
    text = fetch_atom(args.query, args.max_results)
    papers = parse_atom(text)
    write_slice(out, papers, text, args.query)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
