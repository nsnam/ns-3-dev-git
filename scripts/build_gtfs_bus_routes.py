#!/usr/bin/env python3
"""Map scheduled GTFS bus trips onto SUMO bus lanes and write routes/stops."""

from __future__ import annotations

import argparse
import csv
import random
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from datetime import datetime
from io import TextIOWrapper
from pathlib import Path
from typing import Any
from zipfile import ZipFile


def load_table(archive: ZipFile, name: str) -> list[dict[str, str]]:
    member = next((item for item in archive.namelist() if Path(item).name == name), None)
    if member is None:
        raise ValueError(f"GTFS archive is missing {name}")
    with archive.open(member) as stream:
        return list(csv.DictReader(TextIOWrapper(stream, encoding="utf-8-sig")))


def time_to_seconds(value: str) -> int:
    hours, minutes, seconds = value.split(":")
    return int(hours) * 3600 + int(minutes) * 60 + int(float(seconds))


def active_services(archive: ZipFile, service_date: str) -> set[str]:
    calendar = load_table(archive, "calendar.txt")
    exceptions = load_table(archive, "calendar_dates.txt")
    weekday = datetime.strptime(service_date, "%Y%m%d").strftime("%A").lower()
    services = {
        row["service_id"] for row in calendar
        if row.get("start_date", "") <= service_date <= row.get("end_date", "")
        and row.get(weekday) == "1"
    }
    for row in exceptions:
        if row.get("date") == service_date:
            if row.get("exception_type") == "1":
                services.add(row["service_id"])
            elif row.get("exception_type") == "2":
                services.discard(row["service_id"])
    return services


def map_stop(net: Any, stop: dict[str, str], max_distance: float) -> tuple[Any, Any, float, float] | None:
    lon = float(stop["stop_lon"])
    lat = float(stop["stop_lat"])
    point = net.convertLonLat2XY(lon, lat)
    candidates = net.getNeighboringEdges(*point, r=max_distance, includeJunctions=False)
    eligible = sorted(
        ((distance, edge) for edge, distance in candidates
         if edge.allows("bus") and not edge.getID().startswith(":")),
        key=lambda item: (item[0], item[1].getID()),
    )
    for distance, edge in eligible:
        lanes = [lane for lane in edge.getLanes() if lane.allows("bus")]
        if not lanes:
            continue
        lane = min(lanes, key=lambda item: item.getClosestLanePosAndDist(point)[1])
        position = lane.getClosestLanePosAndDist(point)[0]
        return edge, lane, position, distance
    return None


def write_routes(
    network: Path,
    gtfs: Path,
    routes_path: Path,
    stops_path: Path,
    service_date: str,
    begin: int,
    end: int,
    max_stop_distance: float,
    dwell_extra_max: float,
    seed: int,
) -> tuple[int, int]:
    import sumolib

    net = sumolib.net.readNet(str(network))
    with ZipFile(gtfs) as archive:
        services = active_services(archive, service_date)
        trips = load_table(archive, "trips.txt")
        route_rows = load_table(archive, "routes.txt")
        stop_rows = load_table(archive, "stops.txt")
        stop_times = load_table(archive, "stop_times.txt")

    route_by_id = {row["route_id"]: row for row in route_rows}
    stop_by_id = {row["stop_id"]: row for row in stop_rows}
    times_by_trip: dict[str, list[dict[str, str]]] = defaultdict(list)
    for stop_time in stop_times:
        if stop_time.get("trip_id") in times_by_trip or stop_time.get("trip_id"):
            times_by_trip[stop_time["trip_id"]].append(stop_time)
    trips_by_id = {
        row["trip_id"]: row for row in trips
        if row.get("service_id") in services
        and row.get("route_id") in route_by_id
        and route_by_id[row["route_id"]].get("route_type", "").split(".")[0] == "3"
    }

    rng = random.Random(seed)
    mapped_stops: dict[str, tuple[Any, float, float, str]] = {}
    output_root = ET.Element("routes")
    stop_root = ET.Element("additional")
    candidates = 0
    generated = 0
    for trip_id, trip in trips_by_id.items():
        sequence = sorted(times_by_trip.get(trip_id, []), key=lambda item: float(item["stop_sequence"]))
        if len(sequence) < 2:
            continue
        first_depart = time_to_seconds(sequence[0]["departure_time"])
        if not begin <= first_depart < end:
            continue
        candidates += 1

        mapped_sequence: list[tuple[dict[str, str], Any, Any, float, float, float]] = []
        for stop_time in sequence:
            stop = stop_by_id.get(stop_time.get("stop_id", ""))
            if stop is None:
                continue
            mapped = map_stop(net, stop, max_stop_distance)
            if mapped is None:
                continue
            edge, lane, position, distance = mapped
            mapped_sequence.append((stop, edge, lane, position, distance, time_to_seconds(stop_time["arrival_time"])))
        if len(mapped_sequence) < 2:
            continue

        connected_runs: list[tuple[list[tuple[dict[str, str], Any, Any, float, float, float]], list[str]]] = []
        run_stops = [mapped_sequence[0]]
        run_edges = [mapped_sequence[0][1].getID()]
        for current, following in zip(mapped_sequence, mapped_sequence[1:]):
            path, _ = net.getShortestPath(current[1], following[1], vClass="bus")
            if path is None:
                connected_runs.append((run_stops, run_edges))
                run_stops = [following]
                run_edges = [following[1].getID()]
                continue
            run_stops.append(following)
            run_edges.extend(edge.getID() for edge in path[1:])
        connected_runs.append((run_stops, run_edges))
        mapped_sequence, edges = max(connected_runs, key=lambda item: (len(item[0]), len(item[1])))
        if len(mapped_sequence) < 2 or len(edges) < 2:
            continue

        unique_edges: list[str] = []
        for edge_id in edges:
            if not unique_edges or edge_id != unique_edges[-1]:
                unique_edges.append(edge_id)
        vehicle_id = f"guimabus_{trip_id}"
        route_row = route_by_id[trip["route_id"]]
        vehicle = ET.SubElement(output_root, "vehicle", {
            "id": vehicle_id,
            "type": "bus",
            "depart": str(first_depart),
            "line": route_row.get("route_short_name", "") or trip["route_id"],
            "departLane": "best",
            "departSpeed": "max",
        })
        route = ET.SubElement(vehicle, "route", {"edges": " ".join(unique_edges)})
        route_index = 0
        last_stop_end = -1.0
        for sequence_index, item in enumerate(mapped_sequence[1:], start=1):
            stop, edge, lane, position, distance, arrival = item
            edge_id = edge.getID()
            next_route_index = next(
                (index for index in range(route_index, len(unique_edges)) if unique_edges[index] == edge_id),
                None,
            )
            if next_route_index is None or next_route_index < route_index:
                continue
            lane_length = lane.getLength()
            start_pos = max(0.0, min(position - 6.5, lane_length - 1.0))
            end_pos = min(start_pos + 13.0, lane_length)
            if end_pos <= start_pos:
                continue
            if next_route_index == route_index and start_pos < last_stop_end:
                continue
            bus_stop_id = f"gtfs_{trip_id}_{sequence_index}"
            stop_root.append(ET.Element("busStop", {
                "id": bus_stop_id,
                "lane": lane.getID(),
                "startPos": f"{start_pos:.2f}",
                "endPos": f"{end_pos:.2f}",
                "friendlyPos": "true",
                "name": stop.get("stop_name", bus_stop_id),
            }))
            minimum_dwell = 10.0 + rng.uniform(0.0, dwell_extra_max)
            attributes = {"busStop": bus_stop_id, "duration": f"{minimum_dwell:.1f}"}
            if arrival > first_depart:
                attributes["until"] = str(arrival)
            route.append(ET.Element("stop", attributes))
            route_index = next_route_index
            last_stop_end = end_pos
            mapped_stops[stop["stop_id"]] = (lane, start_pos, end_pos, stop.get("stop_name", bus_stop_id))
        generated += 1

    ET.indent(output_root, space="    ")
    ET.indent(stop_root, space="    ")
    routes_path.parent.mkdir(parents=True, exist_ok=True)
    stops_path.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(output_root).write(routes_path, encoding="utf-8", xml_declaration=True)
    ET.ElementTree(stop_root).write(stops_path, encoding="utf-8", xml_declaration=True)
    print(f"GTFS buses: {candidates} departures in window, {generated} routable trips, {len(mapped_stops)} unique stops.")
    return candidates, generated


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--network", type=Path, required=True)
    parser.add_argument("--gtfs", type=Path, required=True)
    parser.add_argument("--routes", type=Path, required=True)
    parser.add_argument("--stops", type=Path, required=True)
    parser.add_argument("--date", required=True)
    parser.add_argument("--begin", type=int, required=True)
    parser.add_argument("--end", type=int, required=True)
    parser.add_argument("--max-stop-distance", type=float, default=1500.0)
    parser.add_argument("--dwell-extra-max", type=float, default=8.0)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    _, generated = write_routes(
        args.network, args.gtfs, args.routes, args.stops, args.date,
        args.begin, args.end, args.max_stop_distance, args.dwell_extra_max, args.seed,
    )
    if generated == 0:
        raise RuntimeError("No scheduled GTFS bus trips could be mapped to the SUMO network")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())