#!/usr/bin/env python3
"""Apply reproducible bus dwell variation and merge SUMO-generated bus types."""

from __future__ import annotations

import argparse
import random
import xml.etree.ElementTree as ET
from pathlib import Path


BUS_ATTRIBUTES = {
    "vClass": "bus",
    "accel": "1.0",
    "decel": "3.0",
    "length": "12.0",
    "minGap": "2.5",
    "maxSpeed": "13.9",
    "sigma": "0.5",
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--routes", type=Path, nargs="+", required=True)
    parser.add_argument("--generated-vtypes", type=Path, nargs="+", required=True)
    parser.add_argument("--vtypes", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--dwell-extra-max", type=float, default=8.0)
    args = parser.parse_args()

    route_trees = [(path, ET.parse(path)) for path in args.routes]
    bus_routes_root = route_trees[0][1].getroot()
    rng = random.Random(args.seed)
    stop_count = 0
    for stop in bus_routes_root.iter("stop"):
        minimum = float(stop.get("duration", "10"))
        stop.set("duration", f"{minimum + rng.uniform(0.0, args.dwell_extra_max):.1f}")
        stop_count += 1

    vtypes_tree = ET.parse(args.vtypes)
    vtypes_root = vtypes_tree.getroot()
    known_types = {item.get("id"): item for item in vtypes_root.iter("vType")}
    for _, route_tree in route_trees:
        routes_root = route_tree.getroot()
        for embedded_type in list(routes_root.findall("vType")):
            if embedded_type.get("id") in known_types:
                routes_root.remove(embedded_type)
    for generated_file in args.generated_vtypes:
        if not generated_file.exists():
            continue
        generated_tree = ET.parse(generated_file)
        for generated in generated_tree.getroot().iter("vType"):
            type_id = generated.get("id")
            if not type_id:
                continue
            target = known_types.get(type_id)
            if target is None:
                target = ET.Element("vType", generated.attrib.copy())
                vtypes_root.append(target)
                known_types[type_id] = target
            if generated.get("vClass") == "bus":
                target.attrib.update(BUS_ATTRIBUTES)

    defined_types = set(known_types)
    referenced_types = {
        vehicle.get("type")
        for _, route_tree in route_trees
        for vehicle in route_tree.getroot().findall("vehicle")
        if vehicle.get("type")
    }
    for type_id in sorted(referenced_types - defined_types):
        attributes = {"id": type_id, **BUS_ATTRIBUTES}
        vtypes_root.append(ET.Element("vType", attributes))
        known_types[type_id] = vtypes_root[-1]

    ET.indent(vtypes_tree, space="    ")
    for route_path, route_tree in route_trees:
        ET.indent(route_tree, space="    ")
        route_tree.write(route_path, encoding="utf-8", xml_declaration=True)
    vtypes_tree.write(args.vtypes, encoding="utf-8", xml_declaration=True)
    print(f"Prepared {len(referenced_types)} bus types and randomized {stop_count} stop dwells (seed={args.seed}).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())