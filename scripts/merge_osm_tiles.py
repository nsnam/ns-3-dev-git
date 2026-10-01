#!/usr/bin/env python3
"""Merge tiled Overpass OSM XML files into one de-duplicated OSM document."""

from __future__ import annotations

import argparse
import copy
import xml.etree.ElementTree as ET
from pathlib import Path


OSM_ELEMENTS = {"node", "way", "relation"}


def merge_files(inputs: list[Path], output: Path) -> dict[str, int]:
    root = ET.Element("osm", {"version": "0.6", "generator": "ns3-guimaraes-mobility"})
    elements: dict[tuple[str, str], ET.Element] = {}
    for path in inputs:
        tile = ET.parse(path).getroot()
        for element in tile:
            if element.tag not in OSM_ELEMENTS:
                continue
            element_id = element.get("id")
            if element_id is None:
                continue
            elements.setdefault((element.tag, element_id), copy.deepcopy(element))

    if not elements:
        raise RuntimeError("No OSM nodes, ways or relations found in the downloaded tiles")

    for element in elements.values():
        root.append(element)
    ET.indent(root, space=" ")
    output.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(root).write(output, encoding="utf-8", xml_declaration=True)
    counts = {kind: sum(tag == kind for tag, _ in elements) for kind in sorted(OSM_ELEMENTS)}
    print(f"Merged {len(inputs)} Overpass tiles into {output}: {counts}")
    return counts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("inputs", type=Path, nargs="+")
    args = parser.parse_args()

    merge_files(args.inputs, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())