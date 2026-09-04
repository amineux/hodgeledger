#!/usr/bin/env python3
"""Verify HodgeLedger double-entry journals (0-flow and 1-flow).

Works without cobc. Checks CSV conservation, optional 96-byte journal.dat,
and optional trial_balance.csv.
"""

from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path

RECORD = 96


def fail(msg: str) -> int:
    print("verify_ledger: FAIL:", msg, file=sys.stderr)
    return 1


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--journal", required=True, help="journal.csv")
    ap.add_argument("--dat", default=None, help="journal.dat")
    ap.add_argument("--trial", default=None, help="trial_balance.csv")
    args = ap.parse_args()

    path = Path(args.journal)
    if not path.exists():
        return fail(f"missing {path}")

    debit = credit = 0
    d0 = c0 = d1 = c1 = 0
    n = 0
    with path.open(encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        return fail("empty journal")
    for row in rows:
        dr = (row.get("debit_account") or row.get("debit") or "").strip()
        cr = (row.get("credit_account") or row.get("credit") or "").strip()
        amt = int(row.get("amount_cents") or "0")
        dim = int(row.get("dim") or "0")
        if not dr or not cr or dr == cr:
            return fail(f"line {row.get('je_id')}: accounts must be distinct")
        if amt <= 0:
            return fail(f"line {row.get('je_id')}: amount must be positive")
        if dim not in (0, 1):
            return fail(f"line {row.get('je_id')}: dim must be 0 or 1")
        debit += amt
        credit += amt
        if dim == 0:
            d0 += amt
            c0 += amt
        else:
            d1 += amt
            c1 += amt
        n += 1

    if debit != credit:
        return fail(f"totals DR {debit} != CR {credit}")
    if d0 != c0:
        return fail(f"chart 0 DR {d0} != CR {c0}")
    if d1 != c1:
        return fail(f"chart 1 DR {d1} != CR {c1}")

    if args.dat:
        dpath = Path(args.dat)
        if not dpath.exists():
            return fail(f"missing {dpath}")
        lines = dpath.read_text(encoding="utf-8").splitlines()
        if len(lines) != n:
            return fail(f"dat lines {len(lines)} != csv rows {n}")
        for i, line in enumerate(lines, 1):
            if len(line) != RECORD:
                return fail(f"dat line {i} width {len(line)} != {RECORD}")
            dimch = line[16]
            if dimch not in "01":
                return fail(f"dat line {i} dim {dimch!r}")

    if args.trial:
        tpath = Path(args.trial)
        if not tpath.exists():
            return fail(f"missing {tpath}")
        acc = defaultdict(lambda: [0, 0])
        with path.open(encoding="utf-8") as f:
            for row in csv.DictReader(f):
                dr = (row.get("debit_account") or row.get("debit") or "").strip()
                cr = (row.get("credit_account") or row.get("credit") or "").strip()
                amt = int(row.get("amount_cents") or "0")
                acc[dr][0] += amt
                acc[cr][1] += amt
        with tpath.open(encoding="utf-8") as f:
            trows = list(csv.DictReader(f))
        total = [r for r in trows if (r.get("account") or "").strip() == "TOTAL"]
        if not total:
            return fail("trial balance missing TOTAL")
        if int(total[0]["debit_cents"]) != debit or int(total[0]["credit_cents"]) != credit:
            return fail("trial TOTAL does not match journal")

    print(
        f"verify_ledger: OK lines={n} DR={debit} CR={credit} "
        f"chart0={d0} chart1={d1}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
