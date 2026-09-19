#!/usr/bin/env python3
"""
tools/seed.py — deterministic per-cell seeds for ndm-sys scenario runs (D7).

Contract:
  seed = first 8 bytes of SHA-256(cell_id encoded as UTF-8), read as a big-endian
  unsigned 64-bit integer, with the top bit cleared (63-bit positive seed).

Properties:
  - stable across platforms, Python versions and runs (no wall clock, no random);
  - same cell id => same seed, always (reproducible retries);
  - different cell ids => (for any practical set of cells) different seeds.

The seed feeds ns3::RngStream in the simulation; run_matrix.py (Phase 8) calls
`python3 tools/seed.py <cell-id>` for every cell and records it in the manifest.

Usage:
  python3 tools/seed.py <cell-id> [<cell-id> ...]
  python3 tools/seed.py --hex <cell-id>
"""

import hashlib
import sys


def cell_seed(cell_id: str) -> int:
    """Return the stable 63-bit seed for a cell id (string)."""
    if not isinstance(cell_id, str) or cell_id == "":
        raise ValueError("cell id must be a non-empty string")
    digest = hashlib.sha256(cell_id.encode("utf-8")).digest()
    return int.from_bytes(digest[:8], "big") & 0x7FFFFFFFFFFFFFFF


def main(argv):
    as_hex = False
    if argv and argv[0] == "--hex":
        as_hex = True
        argv = argv[1:]
    if not argv:
        print(__doc__.strip().splitlines()[-4], file=sys.stderr)
        return 2
    for cell_id in argv:
        s = cell_seed(cell_id)
        if as_hex:
            print(f"{cell_id}\t{s:016x}")
        else:
            print(f"{cell_id}\t{s}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
