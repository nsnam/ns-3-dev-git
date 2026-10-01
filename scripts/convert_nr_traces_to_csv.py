#!/usr/bin/env python3
"""Convert tab-separated native 5G-LENA trace files to CSV files."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def convert(source: Path) -> Path | None:
    with source.open("r", encoding="utf-8", errors="replace", newline="") as input_file:
        rows = list(csv.reader(input_file, delimiter="\t"))
    rows = [row for row in rows if row and any(field.strip() for field in row)]
    if not rows or not any(len(row) > 1 for row in rows):
        return None
    rows[0][0] = rows[0][0].lstrip("% ").strip()
    output = source.with_suffix(".csv")
    with output.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file)
        writer.writerows(rows)
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, default=Path("results/raw_data"))
    args = parser.parse_args()

    converted = 0
    sources = sorted(set(args.directory.glob("*.txt")) | set(args.directory.glob("*.tsv")))
    for source in sources:
        if convert(source) is not None:
            converted += 1
    print(f"Converted {converted} native NR TSV traces to CSV in {args.directory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())