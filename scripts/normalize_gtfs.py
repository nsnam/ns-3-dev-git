#!/usr/bin/env python3
"""Apply SUMO-compatible defaults to optional Guimabus GTFS text fields."""

from __future__ import annotations

import argparse
import csv
from io import StringIO
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


def normalize_routes(data: bytes, names_by_route: dict[str, str] | None = None) -> tuple[bytes, int]:
    text = data.decode("utf-8-sig")
    reader = csv.DictReader(StringIO(text))
    if not reader.fieldnames:
        raise ValueError("routes.txt has no header")
    rows = list(reader)
    if "route_short_name" not in reader.fieldnames:
        reader.fieldnames.append("route_short_name")
    filled = 0
    for row in rows:
        short_name = row.get("route_short_name", "").strip()
        if short_name.lower() in {"", "nan", "none", "null"}:
            long_name = row.get("route_long_name", "").strip()
            if long_name.lower() in {"", "nan", "none", "null"}:
                route_id = row.get("route_id", "")
                long_name = (names_by_route or {}).get(route_id, "") or route_id
            row["route_short_name"] = long_name
            filled += 1
    output = StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=reader.fieldnames, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode("utf-8"), filled


def remove_duplicate_route_name(data: bytes) -> tuple[bytes, bool]:
    reader = csv.DictReader(StringIO(data.decode("utf-8-sig")))
    if not reader.fieldnames or "route_short_name" not in reader.fieldnames:
        return data, False
    fieldnames = [field for field in reader.fieldnames if field != "route_short_name"]
    output = StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    for row in reader:
        writer.writerow({field: row.get(field, "") for field in fieldnames})
    return output.getvalue().encode("utf-8"), True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    filled_counts: dict[str, int] = {}
    with ZipFile(args.input) as source:
        routes_member = next((item for item in source.namelist() if Path(item).name == "routes.txt"), None)
        if routes_member is None:
            raise ValueError("GTFS archive has no routes.txt")
        routes_data, _ = normalize_routes(source.read(routes_member))
        routes_reader = csv.DictReader(StringIO(routes_data.decode("utf-8")))
        names_by_route = {row["route_id"]: row["route_short_name"] for row in routes_reader}
        with ZipFile(args.output, "w", ZIP_DEFLATED) as target:
            for item in source.infolist():
                data = source.read(item.filename)
                base_name = Path(item.filename).name
                if base_name == "routes.txt":
                    data, filled_counts[base_name] = normalize_routes(data)
                elif base_name in {"trips.txt", "stop_times.txt"}:
                    data, removed = remove_duplicate_route_name(data)
                    filled_counts[base_name] = int(removed)
                target.writestr(item, data)

    source_note = args.input.with_suffix(".source.txt")
    output_note = args.output.with_suffix(".source.txt")
    source_text = source_note.read_text(encoding="utf-8") if source_note.exists() else f"source={args.input}\n"
    output_note.write_text(
        source_text + "sumo_route_short_names_filled="
        + ",".join(f"{name}:{count}" for name, count in sorted(filled_counts.items())) + "\n",
        encoding="utf-8",
    )
    print(f"Prepared SUMO GTFS copy: {args.output} (route_short_name filled: {filled_counts})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())