#!/usr/bin/env python3
"""Filter OpenCellID NOS cell records and project them into SUMO's local XY."""

from __future__ import annotations

import argparse
import csv
import json
import os
import sys
from io import StringIO
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen


DEFAULT_BBOX = (41.43, -8.32, 41.47, -8.27)
DEFAULT_API_URL = "https://opencellid.org/cell/getInArea"
RADIOS = {"LTE", "NR"}
ATTRIBUTION = (
    "Cell tower data from OpenCellID (https://opencellid.org/), licensed under "
    "CC BY-SA 4.0 (https://creativecommons.org/licenses/by-sa/4.0/)."
)


def value(row: dict[str, Any], *names: str, default: Any = "") -> Any:
    normalized = {str(key).strip().lower(): item for key, item in row.items() if key is not None}
    for name in names:
        candidate = normalized.get(name.lower())
        if candidate is not None and str(candidate).strip() not in {"", "None", "nan", "null"}:
            return candidate
    return default


def as_int(raw: Any) -> int:
    return int(float(str(raw).strip()))


def parse_csv(data: str) -> list[dict[str, Any]]:
    return [dict(row) for row in csv.DictReader(StringIO(data))]


def parse_records(data: str, input_format: str) -> list[dict[str, Any]]:
    if input_format == "csv":
        return parse_csv(data)
    parsed = json.loads(data)
    if isinstance(parsed, list):
        return [dict(item) for item in parsed if isinstance(item, dict)]
    if isinstance(parsed, dict):
        cells = parsed.get("cells")
        if isinstance(cells, list):
            return [dict(item) for item in cells if isinstance(item, dict)]
        if "stat" in parsed and parsed.get("stat") != "ok":
            raise RuntimeError(f"OpenCellID rejected the query: {parsed.get('err', parsed)}")
    raise ValueError("Expected a CSV table, a JSON list, or an OpenCellID JSON object with a cells array")


def read_input(path: Path, input_format: str) -> list[dict[str, Any]]:
    data = sys.stdin.read() if str(path) == "-" else path.read_text(encoding="utf-8-sig")
    selected_format = input_format
    if selected_format == "auto":
        selected_format = "csv" if data.lstrip().lower().startswith(("lat,", "radio,", "mcc,")) else "json"
    return parse_records(data, selected_format)


def fetch_api_records(api_key: str, bbox: tuple[float, float, float, float], api_url: str) -> list[dict[str, Any]]:
    lat_min, lon_min, lat_max, lon_max = bbox
    records: list[dict[str, Any]] = []
    tile_rows = 3
    tile_columns = 3
    for row_index in range(tile_rows):
        tile_lat_min = lat_min + (lat_max - lat_min) * row_index / tile_rows
        tile_lat_max = lat_min + (lat_max - lat_min) * (row_index + 1) / tile_rows
        for column_index in range(tile_columns):
            tile_lon_min = lon_min + (lon_max - lon_min) * column_index / tile_columns
            tile_lon_max = lon_min + (lon_max - lon_min) * (column_index + 1) / tile_columns
            for radio in sorted(RADIOS):
                offset = 0
                while True:
                    query = urlencode({
                        "key": api_key,
                        "BBOX": f"{tile_lat_min},{tile_lon_min},{tile_lat_max},{tile_lon_max}",
                        "mcc": 268,
                        "mnc": 3,
                        "radio": radio,
                        "format": "json",
                        "limit": 50,
                        "offset": offset,
                    })
                    request = Request(f"{api_url}?{query}", headers={"User-Agent": "ns3-guimaraes-v2x/1.0"})
                    try:
                        with urlopen(request, timeout=30) as response:
                            payload = json.loads(response.read().decode("utf-8"))
                    except HTTPError as error:
                        detail = error.read().decode("utf-8", errors="replace").strip()
                        detail = detail.replace(api_key, "<redacted>")
                        raise RuntimeError(
                            f"OpenCellID API returned HTTP {error.code}: {detail or 'empty error response'}"
                        ) from error
                    except URLError as error:
                        raise RuntimeError(f"Could not reach OpenCellID API: {error.reason}") from error
                    if not isinstance(payload, dict) or payload.get("stat", "ok") != "ok":
                        raise RuntimeError(
                            f"OpenCellID API error: {payload.get('err', payload) if isinstance(payload, dict) else payload}"
                        )
                    page = payload.get("cells", [])
                    if not isinstance(page, list):
                        raise ValueError("OpenCellID API response has no cells array")
                    records.extend(dict(item) for item in page if isinstance(item, dict))
                    offset += len(page)
                    total = int(payload.get("count", 0))
                    if not page or offset >= total or len(page) < 50:
                        break
    return records


def resolve_sumolib() -> Any:
    sumo_home = os.environ.get("SUMO_HOME")
    candidates = [Path(sumo_home) / "tools"] if sumo_home else []
    candidates.extend([Path("/usr/share/sumo/tools"), Path("/usr/local/share/sumo/tools")])
    for tools in candidates:
        if (tools / "sumolib").is_dir():
            sys.path.insert(0, str(tools))
            import sumolib

            return sumolib
    raise RuntimeError("SUMO Python tools not found; set SUMO_HOME before parsing gNB positions")


def normalize_cell(row: dict[str, Any], bbox: tuple[float, float, float, float]) -> dict[str, Any] | None:
    try:
        mcc = as_int(value(row, "mcc"))
        mnc = as_int(value(row, "mnc", "net", "network"))
        latitude = float(value(row, "lat", "latitude"))
        longitude = float(value(row, "lon", "lng", "longitude"))
        cell_id = str(value(row, "cellid", "cell_id", "cell"))
        radio = str(value(row, "radio", "technology", default="LTE")).strip().upper()
    except (TypeError, ValueError):
        return None
    lat_min, lon_min, lat_max, lon_max = bbox
    if (mcc != 268 or mnc != 3 or radio not in RADIOS or not cell_id
            or not lat_min <= latitude <= lat_max or not lon_min <= longitude <= lon_max):
        return None
    try:
        lac = as_int(value(row, "lac", "tac", default=0))
    except (TypeError, ValueError):
        lac = 0
    try:
        cell_range = float(value(row, "range", "range_m", default=0))
    except (TypeError, ValueError):
        cell_range = 0.0
    try:
        samples = as_int(value(row, "samples", default=0))
    except (TypeError, ValueError):
        samples = 0
    return {
        "cell_id": cell_id,
        "radio": radio,
        "mcc": mcc,
        "mnc": mnc,
        "lac_tac": lac,
        "latitude": latitude,
        "longitude": longitude,
        "range_m": cell_range,
        "samples": samples,
    }


def project_records(
    rows: list[dict[str, Any]], bbox: tuple[float, float, float, float], network_path: Path
) -> list[dict[str, Any]]:
    sumolib = resolve_sumolib()
    net = sumolib.net.readNet(str(network_path))
    projected: dict[tuple[str, int, int, str], dict[str, Any]] = {}
    for row in rows:
        cell = normalize_cell(row, bbox)
        if cell is None:
            continue
        x, y = net.convertLonLat2XY(cell["longitude"], cell["latitude"])
        cell.update({"x": round(x, 3), "y": round(y, 3), "z": 15.0})
        key = (cell["radio"], cell["mcc"], cell["mnc"], cell["cell_id"])
        projected[key] = cell
    return sorted(projected.values(), key=lambda item: (item["radio"], item["cell_id"]))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, help="OpenCellID CSV/JSON download; use - for stdin")
    parser.add_argument("--format", choices=("auto", "csv", "json"), default="auto")
    parser.add_argument("--output", type=Path, default=Path("mobility/guimaraes/gnb_positions.json"))
    parser.add_argument("--network", type=Path, default=Path("mobility/guimaraes/guimaraes.net.xml"))
    parser.add_argument("--bbox", default="41.43,-8.32,41.47,-8.27", help="lat_min,lon_min,lat_max,lon_max")
    parser.add_argument("--api-url", default=DEFAULT_API_URL)
    parser.add_argument("--api-key-env", default="OPENCELLID_API_KEY")
    args = parser.parse_args()
    bbox = tuple(float(part) for part in args.bbox.split(","))
    if len(bbox) != 4 or bbox[0] >= bbox[2] or bbox[1] >= bbox[3]:
        parser.error("bbox must be lat_min,lon_min,lat_max,lon_max")

    if args.input:
        rows = read_input(args.input, args.format)
        source = str(args.input)
    else:
        api_key = os.environ.get(args.api_key_env, "")
        if not api_key:
            parser.error(f"Set {args.api_key_env} or supply --input with an OpenCellID CSV/JSON download")
        rows = fetch_api_records(api_key, bbox, args.api_url)
        source = args.api_url

    cells = project_records(rows, bbox, args.network)
    if not cells:
        raise RuntimeError("No NOS LTE/NR OpenCellID cells matched the requested bounding box")
    result = {
        "source": source,
        "operator": "NOS Portugal",
        "mcc": 268,
        "mnc": 3,
        "bbox": {"lat_min": bbox[0], "lon_min": bbox[1], "lat_max": bbox[2], "lon_max": bbox[3]},
        "coordinate_system": "SUMO guimaraes.net.xml projected local XY in metres",
        "gNB_altitude_m": 15.0,
        "attribution": ATTRIBUTION,
        "note": "OpenCellID entries are logical cell records; several sectors may share one physical site.",
        "gnbs": cells,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OpenCellID: wrote {len(cells)} NOS LTE/NR cell positions to {args.output}")
    print(ATTRIBUTION)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())