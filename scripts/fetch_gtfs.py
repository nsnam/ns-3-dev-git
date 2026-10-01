#!/usr/bin/env python3
"""Download the Guimabus GTFS feed, falling back to a small valid demo feed."""

from __future__ import annotations

import argparse
import csv
from io import BytesIO, StringIO
from pathlib import Path
from typing import Any
from urllib.request import Request, urlopen
from zipfile import ZIP_DEFLATED, ZipFile


DEFAULT_URL = (
    "https://files.mobilitydatabase.org/mdb-2838/mdb-2838-202512190141/"
    "mdb-2838-202512190141.zip"
)
REQUIRED_FILES = (
    "agency.txt",
    "calendar.txt",
    "calendar_dates.txt",
    "routes.txt",
    "stops.txt",
    "stop_times.txt",
    "trips.txt",
)


def read_table(archive: ZipFile, name: str) -> list[dict[str, str]]:
    member = next((item for item in archive.namelist() if Path(item).name == name), None)
    if member is None:
        raise ValueError(f"GTFS file is missing: {name}")
    text = archive.read(member).decode("utf-8-sig")
    return list(csv.DictReader(StringIO(text)))


def validate_feed(data: bytes) -> dict[str, int]:
    try:
        archive = ZipFile(BytesIO(data))
    except Exception as error:
        raise ValueError("download is not a readable ZIP archive") from error

    with archive:
        names = {Path(item).name for item in archive.namelist()}
        missing = sorted(set(REQUIRED_FILES) - names)
        if missing:
            raise ValueError(f"GTFS is missing required tables: {', '.join(missing)}")

        tables = {name: read_table(archive, name) for name in REQUIRED_FILES}
        for name in REQUIRED_FILES:
            if not tables[name] and name != "calendar_dates.txt":
                raise ValueError(f"GTFS table has no records: {name}")

        agency_ids = {row.get("agency_id", "") for row in tables["agency.txt"]}
        route_ids = {row.get("route_id", "") for row in tables["routes.txt"]}
        stop_ids = {row.get("stop_id", "") for row in tables["stops.txt"]}
        trip_ids = {row.get("trip_id", "") for row in tables["trips.txt"]}
        service_ids = {row.get("service_id", "") for row in tables["calendar.txt"]}
        for row in tables["routes.txt"]:
            if row.get("agency_id") and row["agency_id"] not in agency_ids:
                raise ValueError(f"route references unknown agency: {row['route_id']}")
        for row in tables["trips.txt"]:
            if row.get("route_id") not in route_ids or row.get("service_id") not in service_ids:
                raise ValueError(f"trip references an unknown route or service: {row.get('trip_id')}")
        for row in tables["stop_times.txt"]:
            if row.get("trip_id") not in trip_ids or row.get("stop_id") not in stop_ids:
                raise ValueError(f"stop time references an unknown trip or stop: {row.get('trip_id')}")

        return {
            "agencies": len(tables["agency.txt"]),
            "routes": len(tables["routes.txt"]),
            "stops": len(tables["stops.txt"]),
            "trips": len(tables["trips.txt"]),
            "stop_times": len(tables["stop_times.txt"]),
        }


def write_csv(archive: ZipFile, name: str, fields: list[str], rows: list[dict[str, Any]]) -> None:
    buffer = StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    archive.writestr(name, buffer.getvalue())


def create_demo_feed() -> bytes:
    buffer = BytesIO()
    with ZipFile(buffer, "w", ZIP_DEFLATED) as archive:
        write_csv(
            archive,
            "agency.txt",
            ["agency_id", "agency_name", "agency_url", "agency_timezone", "agency_lang"],
            [{
                "agency_id": "GUIMABUS_DEMO",
                "agency_name": "Guimabus (feed demonstrativo)",
                "agency_url": "https://guimabus.pt/",
                "agency_timezone": "Europe/Lisbon",
                "agency_lang": "pt",
            }],
        )
        write_csv(
            archive,
            "calendar.txt",
            ["service_id", "monday", "tuesday", "wednesday", "thursday", "friday",
             "saturday", "sunday", "start_date", "end_date"],
            [{"service_id": "WEEKDAY", "monday": 1, "tuesday": 1, "wednesday": 1,
              "thursday": 1, "friday": 1, "saturday": 0, "sunday": 0,
              "start_date": "20260101", "end_date": "20301231"}],
        )
        write_csv(
            archive,
            "calendar_dates.txt",
            ["service_id", "date", "exception_type"],
            [],
        )
        write_csv(
            archive,
            "routes.txt",
            ["route_id", "agency_id", "route_short_name", "route_long_name", "route_type"],
            [
                {"route_id": "DEMO_AZUREM", "agency_id": "GUIMABUS_DEMO", "route_short_name": "D1",
                 "route_long_name": "Terminal - Centro - Azurém", "route_type": 3},
                {"route_id": "DEMO_CREIXOMIL", "agency_id": "GUIMABUS_DEMO", "route_short_name": "D2",
                 "route_long_name": "Terminal - Veiga de Creixomil", "route_type": 3},
            ],
        )
        stops = [
            {"stop_id": "TERMINAL", "stop_name": "Fornos da Cruz de Pedra", "stop_lat": 41.438110,
             "stop_lon": -8.302325, "location_type": 0},
            {"stop_id": "CENTRO", "stop_name": "Camara Municipal", "stop_lat": 41.445171,
             "stop_lon": -8.292017, "location_type": 0},
            {"stop_id": "AZUREM", "stop_name": "EB23 Joao de Meira - Azurem", "stop_lat": 41.445804,
             "stop_lon": -8.284604, "location_type": 0},
            {"stop_id": "CREIXOMIL", "stop_name": "Selho de Fora - Creixomil", "stop_lat": 41.439822,
             "stop_lon": -8.321303, "location_type": 0},
        ]
        write_csv(
            archive,
            "stops.txt",
            ["stop_id", "stop_name", "stop_lat", "stop_lon", "location_type"],
            stops,
        )
        trips = [
            {"route_id": "DEMO_AZUREM", "service_id": "WEEKDAY", "trip_id": "DEMO_AZUREM_0700",
             "direction_id": 0, "shape_id": "SHAPE_AZUREM", "trip_headsign": "Azurém"},
            {"route_id": "DEMO_CREIXOMIL", "service_id": "WEEKDAY", "trip_id": "DEMO_CREIXOMIL_0702",
             "direction_id": 0, "shape_id": "SHAPE_CREIXOMIL", "trip_headsign": "Creixomil"},
        ]
        write_csv(
            archive,
            "trips.txt",
            ["route_id", "service_id", "trip_id", "direction_id", "shape_id", "trip_headsign"],
            trips,
        )
        write_csv(
            archive,
            "stop_times.txt",
            ["trip_id", "arrival_time", "departure_time", "stop_id", "stop_sequence", "timepoint"],
            [
                {"trip_id": "DEMO_AZUREM_0700", "arrival_time": "07:00:00", "departure_time": "07:00:00",
                 "stop_id": "TERMINAL", "stop_sequence": 1, "timepoint": 1},
                {"trip_id": "DEMO_AZUREM_0700", "arrival_time": "07:05:00", "departure_time": "07:05:30",
                 "stop_id": "CENTRO", "stop_sequence": 2, "timepoint": 1},
                {"trip_id": "DEMO_AZUREM_0700", "arrival_time": "07:15:00", "departure_time": "07:15:30",
                 "stop_id": "AZUREM", "stop_sequence": 3, "timepoint": 1},
                {"trip_id": "DEMO_CREIXOMIL_0702", "arrival_time": "07:02:00", "departure_time": "07:02:00",
                 "stop_id": "CREIXOMIL", "stop_sequence": 1, "timepoint": 1},
                {"trip_id": "DEMO_CREIXOMIL_0702", "arrival_time": "07:10:00", "departure_time": "07:10:30",
                 "stop_id": "TERMINAL", "stop_sequence": 2, "timepoint": 1},
            ],
        )
        write_csv(
            archive,
            "shapes.txt",
            ["shape_id", "shape_pt_lat", "shape_pt_lon", "shape_pt_sequence"],
            [
                {"shape_id": "SHAPE_AZUREM", "shape_pt_lat": 41.438110, "shape_pt_lon": -8.302325,
                 "shape_pt_sequence": 1},
                {"shape_id": "SHAPE_AZUREM", "shape_pt_lat": 41.445171, "shape_pt_lon": -8.292017,
                 "shape_pt_sequence": 2},
                {"shape_id": "SHAPE_AZUREM", "shape_pt_lat": 41.445804, "shape_pt_lon": -8.284604,
                 "shape_pt_sequence": 3},
                {"shape_id": "SHAPE_CREIXOMIL", "shape_pt_lat": 41.439822, "shape_pt_lon": -8.321303,
                 "shape_pt_sequence": 1},
                {"shape_id": "SHAPE_CREIXOMIL", "shape_pt_lat": 41.438110, "shape_pt_lon": -8.302325,
                 "shape_pt_sequence": 2},
            ],
        )
    return buffer.getvalue()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default=DEFAULT_URL, help="public GTFS ZIP URL")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "mobility/guimaraes/guimabus_gtfs.zip",
    )
    parser.add_argument("--demo", action="store_true", help="write the valid local demo feed without downloading")
    args = parser.parse_args()

    source = "synthetic demo feed"
    if args.demo:
        data = create_demo_feed()
    else:
        try:
            request = Request(args.url, headers={"User-Agent": "ns3-guimaraes-mobility/1.0"})
            with urlopen(request, timeout=60) as response:
                data = response.read()
            validate_feed(data)
            source = args.url
        except Exception as error:
            print(f"GTFS download/validation failed ({error}); creating a valid demo feed.")
            data = create_demo_feed()

    counts = validate_feed(data)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    source_path = args.output.with_suffix(".source.txt")
    source_path.write_text(f"source={source}\ncounts={counts}\n", encoding="utf-8")
    print(f"GTFS source: {source}")
    print("GTFS counts: " + ", ".join(f"{key}={value}" for key, value in counts.items()))
    print(f"GTFS archive: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())