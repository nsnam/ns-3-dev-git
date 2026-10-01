#!/usr/bin/env python3
"""Download an adaptive, cached OSM extract using the Overpass Turbo API endpoint."""

from __future__ import annotations

import argparse
import gzip
import math
import time
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Final
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen


DEFAULT_BBOX: Final = (-8.36, 41.40, -8.25, 41.47)
DEFAULT_ENDPOINTS: Final = (
    "https://overpass-api.de/api/interpreter",
    "https://osm.hpi.de/overpass/api/interpreter",
    "https://overpass.private.coffee/api/interpreter",
)
_last_request_at = 0.0


class OverpassRateLimitError(RuntimeError):
    def __init__(self, message: str, retry_after: float) -> None:
        super().__init__(message)
        self.retry_after = retry_after


def make_query(west: float, south: float, east: float, north: float) -> str:
    bbox = f"{south:.7f},{west:.7f},{north:.7f},{east:.7f}"
    return f"""[out:xml][timeout:180];
(
  way[highway]({bbox});
  node[highway=bus_stop]({bbox});
  node[public_transport~"stop_position|platform"]({bbox});
  relation[type=route][route~"bus|trolleybus"]({bbox});
  relation[type=route_master][route_master~"bus|trolleybus"]({bbox});
);
(._;>;);
out body;"""


def request_tile(query: str, endpoint: str, timeout: int) -> bytes:
    global _last_request_at
    interval = 2.0 - (time.monotonic() - _last_request_at)
    if interval > 0:
        time.sleep(interval)
    _last_request_at = time.monotonic()
    request = Request(
        endpoint,
        data=urlencode({"data": query}).encode("ascii"),
        headers={
            "Accept": "application/xml",
            "Accept-Encoding": "gzip",
            "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
            "User-Agent": "ns3-guimaraes-mobility/1.0 (Overpass Turbo query export)",
        },
    )
    try:
        with urlopen(request, timeout=timeout) as response:
            data = response.read()
            if response.headers.get("Content-Encoding") == "gzip":
                data = gzip.decompress(data)
    except HTTPError as error:
        if error.code == 429:
            retry_after = float(error.headers.get("Retry-After", "15"))
            raise OverpassRateLimitError(str(error), retry_after) from error
        raise
    root = ET.fromstring(data)
    if root.tag != "osm":
        raise ValueError(f"Unexpected Overpass response root: {root.tag}")
    remark = root.findtext("remark")
    if remark:
        raise RuntimeError(f"Overpass returned an incomplete result: {remark}")
    if not any(element.tag in {"node", "way", "relation"} for element in root):
        raise ValueError("Overpass tile contains no OSM features")
    return data


def fetch_tile(query: str, endpoints: tuple[str, ...], timeout: int, attempts: int) -> bytes:
    errors: list[str] = []
    for attempt in range(attempts):
        retry_after = 0.0
        for endpoint in endpoints:
            try:
                return request_tile(query, endpoint, timeout)
            except OverpassRateLimitError as error:
                errors.append(f"{endpoint}: {error}")
                retry_after = max(retry_after, error.retry_after)
            except (HTTPError, URLError, TimeoutError, OSError, ET.ParseError, ValueError, RuntimeError) as error:
                errors.append(f"{endpoint}: {error}")
        if attempt + 1 < attempts:
            time.sleep(max(retry_after, min(5 * (2 ** attempt), 30)))
    raise RuntimeError("; ".join(errors[-len(endpoints):]))


def split_bbox(bbox: tuple[float, float, float, float]) -> tuple[tuple[float, float, float, float], ...]:
    west, south, east, north = bbox
    width_m = (east - west) * math.cos(math.radians((south + north) / 2))
    height_m = north - south
    if width_m >= height_m:
        middle = (west + east) / 2
        return (west, south, middle, north), (middle, south, east, north)
    middle = (south + north) / 2
    return (west, south, east, middle), (west, middle, east, north)


def download_recursive(
    bbox: tuple[float, float, float, float],
    tile_id: str,
    cache_dir: Path,
    endpoints: tuple[str, ...],
    timeout: int,
    attempts: int,
    max_depth: int,
    depth: int = 0,
) -> list[Path]:
    cached = cache_dir / f"{tile_id}.osm.xml"
    if cached.exists() and cached.stat().st_size > 0:
        try:
            request_tile_cache = ET.parse(cached).getroot()
            if request_tile_cache.tag == "osm" and len(request_tile_cache):
                print(f"Using cached OSM tile {tile_id} ({cached.stat().st_size} bytes)")
                return [cached]
        except ET.ParseError:
            cached.unlink()

    query = make_query(*bbox)
    try:
        data = fetch_tile(query, endpoints, timeout, attempts)
        cached.write_bytes(data)
        print(f"Downloaded OSM tile {tile_id} ({len(data)} bytes)")
        return [cached]
    except RuntimeError as error:
        if depth >= max_depth:
            raise RuntimeError(f"OSM tile {tile_id} failed at maximum split depth: {error}") from error
        print(f"Splitting OSM tile {tile_id} after endpoint errors: {error}")
        output: list[Path] = []
        for child, child_bbox in enumerate(split_bbox(bbox)):
            output.extend(download_recursive(
                child_bbox, f"{tile_id}.{child}", cache_dir, endpoints,
                timeout, attempts, max_depth, depth + 1,
            ))
        return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bbox", default=",".join(map(str, DEFAULT_BBOX)), help="west,south,east,north")
    parser.add_argument("--tiles-x", type=int, default=3)
    parser.add_argument("--tiles-y", type=int, default=1)
    parser.add_argument("--cache-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=90)
    parser.add_argument("--attempts", type=int, default=2)
    parser.add_argument("--max-depth", type=int, default=4)
    parser.add_argument("--endpoint", action="append", dest="endpoints")
    args = parser.parse_args()

    bbox = tuple(float(value) for value in args.bbox.split(","))
    if len(bbox) != 4 or bbox[0] >= bbox[2] or bbox[1] >= bbox[3]:
        parser.error("bbox must be a valid west,south,east,north extent")
    if args.tiles_x < 1 or args.tiles_y < 1:
        parser.error("tile counts must be positive")
    endpoints = tuple(args.endpoints or DEFAULT_ENDPOINTS)
    args.cache_dir.mkdir(parents=True, exist_ok=True)

    west, south, east, north = bbox
    tiles: list[Path] = []
    for row in range(args.tiles_y):
        tile_south = south + (north - south) * row / args.tiles_y
        tile_north = south + (north - south) * (row + 1) / args.tiles_y
        for column in range(args.tiles_x):
            tile_west = west + (east - west) * column / args.tiles_x
            tile_east = west + (east - west) * (column + 1) / args.tiles_x
            tile_id = f"base-{row}-{column}"
            tiles.extend(download_recursive(
                (tile_west, tile_south, tile_east, tile_north), tile_id,
                args.cache_dir, endpoints, args.timeout, args.attempts, args.max_depth,
            ))

    merge_command = [str(path) for path in tiles]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    from merge_osm_tiles import merge_files

    counts = merge_files([Path(path) for path in merge_command], args.output)
    print(f"OSM extract ready: {args.output} ({counts})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())