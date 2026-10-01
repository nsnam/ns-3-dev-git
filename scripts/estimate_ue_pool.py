#!/usr/bin/env python3
"""Estimate the peak simultaneous SUMO vehicle count for a short simulation window."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--sumo-binary", default="sumo")
    parser.add_argument("--start-time", type=float, required=True)
    parser.add_argument("--duration", type=float, required=True)
    parser.add_argument("--margin", type=int, default=4)
    args = parser.parse_args()

    binary = shutil.which(args.sumo_binary)
    if binary is None:
        parser.error(f"SUMO executable not found: {args.sumo_binary}")
    with tempfile.TemporaryDirectory(prefix="guimaraes-ue-pool-") as temp_dir:
        summary = Path(temp_dir) / "summary.xml"
        command = [
            binary,
            "-c",
            str(args.config.resolve()),
            "--begin",
            str(args.start_time),
            "--end",
            str(args.start_time + args.duration),
            "--summary-output",
            str(summary),
            "--no-step-log",
            "true",
            "--duration-log.disable",
            "true",
        ]
        subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
        steps = ET.parse(summary).getroot().findall("step")
    if not steps:
        raise RuntimeError("SUMO produced no summary steps for UE-pool sizing")
    peak = max(
        int(step.get("inserted", "0")) - int(step.get("ended", "0"))
        for step in steps
    )
    pool_size = max(1, peak + max(0, args.margin))
    print(pool_size)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())