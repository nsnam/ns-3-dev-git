#!/usr/bin/env python3
"""Summarize generated vehicle classes and the final SUMO validation step."""

from __future__ import annotations

import argparse
import xml.etree.ElementTree as ET
from pathlib import Path


def vehicle_count(path: Path) -> int:
    return len(ET.parse(path).getroot().findall("vehicle"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--summary-xml", type=Path, required=True)
    args = parser.parse_args()

    counts = {
        "cars": vehicle_count(args.directory / "cars.rou.xml"),
        "buses": vehicle_count(args.directory / "buses.rou.xml"),
        "bicycles": vehicle_count(args.directory / "bikes.rou.xml"),
    }
    expected = sum(counts.values())
    summary_root = ET.parse(args.summary_xml).getroot()
    steps = summary_root.findall("step")
    if not steps:
        raise RuntimeError("SUMO summary contains no simulation steps")
    final = steps[-1]
    loaded = int(final.get("loaded", "0"))
    inserted = int(final.get("inserted", "0"))
    arrived = int(final.get("arrived", "0"))
    running = int(final.get("running", "0"))
    collisions = int(final.get("collisions", "0"))
    teleports = int(final.get("teleports", "0"))
    sumo_log_path = args.directory / "sumo-validation.stderr.log"
    sumo_log = sumo_log_path.read_text(encoding="utf-8", errors="replace") if sumo_log_path.exists() else ""
    sumo_warnings = sum("warning:" in line.lower() for line in sumo_log.splitlines())
    sumo_errors = sum("error:" in line.lower() for line in sumo_log.splitlines())
    gtfs_log_path = args.directory / "gtfs-import.log"
    gtfs_log = gtfs_log_path.read_text(encoding="utf-8", errors="replace") if gtfs_log_path.exists() else ""
    direct_mapping = "GTFS buses:" in gtfs_log
    mapped_trip_line = next((line.strip() for line in gtfs_log.splitlines() if line.startswith("GTFS buses:")), "")
    if min(counts.values()) == 0:
        raise RuntimeError(f"At least one vehicle class has no generated vehicles: {counts}")
    if loaded != expected:
        raise RuntimeError(f"SUMO loaded {loaded} of {expected} generated vehicles; inspect route ordering and validation logs")
    if sumo_errors:
        raise RuntimeError(f"SUMO reported {sumo_errors} errors; inspect {sumo_log_path}")

    active_feed_file = args.directory / "guimabus_gtfs_active.txt"
    active_feed = Path(active_feed_file.read_text(encoding="utf-8").strip()) if active_feed_file.exists() else None
    source_file = active_feed.with_suffix(".source.txt") if active_feed else args.directory / "guimabus_gtfs.source.txt"
    source = source_file.read_text(encoding="utf-8").strip() if source_file.exists() else "unknown"
    report = [
        "Guimaraes SUMO scenario summary",
        f"GTFS: {source}",
        f"Cars: {counts['cars']}",
        f"Buses: {counts['buses']}",
        f"Bicycles: {counts['bicycles']}",
        f"Total vehicles: {expected}",
        f"SUMO loaded/inserted/arrived/running: {loaded}/{inserted}/{arrived}/{running}",
        f"Collisions/teleports: {collisions}/{teleports}",
        f"SUMO stderr warnings/errors: {sumo_warnings}/{sumo_errors}",
        f"Bus route mapper: {'sumolib direct fallback' if direct_mapping else 'gtfs2pt.py'}",
    ]
    if mapped_trip_line:
        report.append(mapped_trip_line)
    report_path = args.directory / "summary.txt"
    report_path.write_text("\n".join(report) + "\n", encoding="utf-8")
    print("\n".join(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())