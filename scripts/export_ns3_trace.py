#!/usr/bin/env python3
"""Run SUMO through TraCI and export ns-2 and per-step mobility traces."""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import shutil
import sys
import xml.etree.ElementTree as ET
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class Sample:
    time_s: float
    vehicle_id: str
    vehicle_class: str
    x_m: float
    y_m: float
    speed_mps: float


def locate_sumo_tools() -> Path:
    configured = os.environ.get("SUMO_HOME")
    candidates = [Path(configured) / "tools"] if configured else []
    candidates.extend([Path("/usr/share/sumo/tools"), Path("/usr/local/share/sumo/tools")])
    for candidate in candidates:
        if (candidate / "traci").is_dir():
            return candidate
    raise RuntimeError("SUMO tools not found; set SUMO_HOME to the SUMO installation")


def read_config_times(config_path: Path) -> tuple[float, float]:
    root = ET.parse(config_path).getroot()
    begin_node = root.find("./time/begin")
    end_node = root.find("./time/end")
    begin = float(begin_node.get("value", "0")) if begin_node is not None else 0.0
    end = float(end_node.get("value", "300")) if end_node is not None else 300.0
    return begin, end


def collect_samples(config_path: Path, sumo_binary: str) -> tuple[list[Sample], dict[str, int], float]:
    tools = locate_sumo_tools()
    sys.path.insert(0, str(tools))
    import traci

    begin, end = read_config_times(config_path)
    binary_path = shutil.which(sumo_binary)
    if binary_path is None:
        raise RuntimeError(f"SUMO binary not found: {sumo_binary}")

    samples: list[Sample] = []
    vehicle_classes: dict[str, str] = {}
    command = [binary_path, "-c", str(config_path.resolve()), "--no-step-log", "true",
               "--duration-log.disable", "true"]
    traci.start(command)
    try:
        while traci.simulation.getTime() < end:
            traci.simulationStep()
            time_s = max(0.0, traci.simulation.getTime() - begin)
            for vehicle_id in sorted(traci.vehicle.getIDList()):
                x_m, y_m = traci.vehicle.getPosition(vehicle_id)
                vehicle_class = traci.vehicle.getVehicleClass(vehicle_id)
                vehicle_classes[vehicle_id] = vehicle_class
                samples.append(Sample(
                    time_s=time_s,
                    vehicle_id=vehicle_id,
                    vehicle_class=vehicle_class,
                    x_m=x_m,
                    y_m=y_m,
                    speed_mps=traci.vehicle.getSpeed(vehicle_id),
                ))
    finally:
        traci.close()

    if not samples:
        raise RuntimeError("SUMO produced no active-vehicle samples")
    return samples, vehicle_classes, end - begin


def write_csv(path: Path, samples: list[Sample], node_ids: dict[str, int]) -> None:
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.writer(output)
        writer.writerow(["time_s", "node_id", "vehicle_id", "vehicle_class", "x_m", "y_m", "z_m", "speed_mps"])
        for sample in samples:
            writer.writerow([
                f"{sample.time_s:.1f}", node_ids[sample.vehicle_id], sample.vehicle_id,
                sample.vehicle_class, f"{sample.x_m:.3f}", f"{sample.y_m:.3f}", "0.0",
                f"{sample.speed_mps:.3f}",
            ])


def write_ns2_trace(path: Path, samples: list[Sample], node_ids: dict[str, int]) -> None:
    by_vehicle: dict[str, list[Sample]] = {vehicle_id: [] for vehicle_id in node_ids}
    for sample in samples:
        by_vehicle[sample.vehicle_id].append(sample)

    events: list[tuple[float, int, float, float, float]] = []
    with path.open("w", encoding="utf-8") as output:
        output.write("# SUMO local XY positions; node indices map to vehicle IDs in the companion CSV.\n")
        output.write("# Ns2MobilityHelper cannot represent node creation/removal; use the CSV for lifecycle-aware replay.\n")
        for vehicle_id, trajectory in by_vehicle.items():
            first = trajectory[0]
            node_id = node_ids[vehicle_id]
            output.write(f"$node_({node_id}) set X_ {first.x_m:.3f}\n")
            output.write(f"$node_({node_id}) set Y_ {first.y_m:.3f}\n")
            output.write(f"$node_({node_id}) set Z_ 0.0\n")
            for previous, current in zip(trajectory, trajectory[1:]):
                elapsed = current.time_s - previous.time_s
                distance = math.hypot(current.x_m - previous.x_m, current.y_m - previous.y_m)
                speed = distance / elapsed if elapsed > 0 else 0.0
                events.append((current.time_s, node_id, current.x_m, current.y_m, speed))
        for time_s, node_id, x_m, y_m, speed in sorted(events):
            output.write(f'$ns_ at {time_s:.1f} "$node_({node_id}) setdest {x_m:.3f} {y_m:.3f} {speed:.3f}"\n')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--config",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "mobility/guimaraes/guimaraes.sumocfg",
    )
    parser.add_argument("--sumo-binary", default="sumo")
    parser.add_argument("--output-prefix", type=Path)
    args = parser.parse_args()

    prefix = args.output_prefix or args.config.parent / "ns3-mobility"
    prefix.parent.mkdir(parents=True, exist_ok=True)
    samples, vehicle_classes, duration = collect_samples(args.config, args.sumo_binary)
    node_ids = {vehicle_id: index for index, vehicle_id in enumerate(vehicle_classes)}
    csv_path = Path(f"{prefix}.csv")
    ns2_path = Path(f"{prefix}.ns2.tcl")
    metadata_path = Path(f"{prefix}.nodes.json")
    write_csv(csv_path, samples, node_ids)
    write_ns2_trace(ns2_path, samples, node_ids)
    counts = Counter(vehicle_classes.values())
    metadata: dict[str, Any] = {
        "simulation_duration_s": duration,
        "sample_interval_s": 0.1,
        "active_vehicle_count": len(vehicle_classes),
        "samples": len(samples),
        "vehicle_classes": dict(sorted(counts.items())),
        "nodes": {str(index): vehicle_id for vehicle_id, index in node_ids.items()},
    }
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(f"Exported {len(samples)} positions for {len(vehicle_classes)} vehicles over {duration:.1f}s.")
    print(f"CSV: {csv_path}\nns-2: {ns2_path}\nnode map: {metadata_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())