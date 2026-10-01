#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCENARIO_DIR="$ROOT_DIR/mobility/guimaraes"
GTFS_TOOLS_DIR=""
SUMO_BINARY="${SUMO_BINARY:-sumo}"
NETCONVERT_BINARY="${NETCONVERT_BINARY:-netconvert}"
DUAROUTER_BINARY="${DUAROUTER_BINARY:-duarouter}"
MAP_BBOX="${MAP_BBOX:--8.36,41.40,-8.25,41.47}"
SIM_DATE="${SIM_DATE:-20260930}"
SIM_BEGIN="${SIM_BEGIN:-25200}"
SIM_END="${SIM_END:-25500}"
CAR_RATE="${CAR_RATE:-600}"
BIKE_RATE="${BIKE_RATE:-120}"

if [[ -n "${PYTHON:-}" ]]; then
    PYTHON_BIN="$PYTHON"
elif [[ -x "$ROOT_DIR/../bin/python" ]]; then
    PYTHON_BIN="$ROOT_DIR/../bin/python"
else
    PYTHON_BIN="$(command -v python3)"
fi

if [[ -n "${SUMO_HOME:-}" ]]; then
    GTFS_TOOLS_DIR="$SUMO_HOME/tools/import/gtfs"
else
    for candidate in /usr/share/sumo /usr/local/share/sumo; do
        if [[ -d "$candidate/tools/import/gtfs" ]]; then
            SUMO_HOME="$candidate"
            GTFS_TOOLS_DIR="$candidate/tools/import/gtfs"
            break
        fi
    done
fi

if [[ -z "$GTFS_TOOLS_DIR" || ! -f "$GTFS_TOOLS_DIR/gtfs2pt.py" ]]; then
    printf 'SUMO GTFS tools not found. Set SUMO_HOME to the SUMO installation.\n' >&2
    exit 1
fi
export SUMO_HOME
export PYTHONPATH="$SUMO_HOME/tools${PYTHONPATH:+:$PYTHONPATH}"

for binary in "$SUMO_BINARY" "$NETCONVERT_BINARY" "$DUAROUTER_BINARY"; do
    if ! command -v "$binary" >/dev/null 2>&1; then
        printf 'Required executable not found: %s\n' "$binary" >&2
        exit 1
    fi
done

if (( SIM_END - SIM_BEGIN != 300 )); then
    printf 'SIM_END - SIM_BEGIN must be 300 seconds (got %s).\n' "$((SIM_END - SIM_BEGIN))" >&2
    exit 1
fi

if ! "$PYTHON_BIN" -c 'import pandas' >/dev/null 2>&1; then
    "$PYTHON_BIN" -m pip install -r "$ROOT_DIR/scripts/requirements-mobility.txt"
fi

mkdir -p "$SCENARIO_DIR" "$SCENARIO_DIR/generated/fcd" "$SCENARIO_DIR/generated/gpsdat"
cd "$ROOT_DIR"

OSM_FILE="$SCENARIO_DIR/guimaraes.osm"
if [[ ! -s "$OSM_FILE" || "${FORCE_MAP_DOWNLOAD:-0}" == "1" ]]; then
    OSM_TILE_DIR="$SCENARIO_DIR/generated/osm-tiles"
    "$PYTHON_BIN" "$ROOT_DIR/scripts/download_guimaraes_osm.py" \
        --bbox="$MAP_BBOX" --tiles-x "${OSM_TILES_X:-3}" --tiles-y "${OSM_TILES_Y:-1}" \
        --cache-dir "$OSM_TILE_DIR" --output "$OSM_FILE"
fi

"$NETCONVERT_BINARY" \
    --osm-files "$OSM_FILE" \
    --output-file "$SCENARIO_DIR/guimaraes.net.xml" \
    --crossings.guess \
    --sidewalks.guess \
    --ptstop-output "$SCENARIO_DIR/ptstops.xml" \
    --ptline-output "$SCENARIO_DIR/ptlines.xml" \
    --geometry.remove \
    --ramps.guess \
    --junctions.corner-detail 5

"$PYTHON_BIN" "$ROOT_DIR/scripts/fetch_gtfs.py" \
    --output "$SCENARIO_DIR/guimabus_gtfs.zip"

GTFS_DATE="$SIM_DATE"
GTFS_BEGIN="$SIM_BEGIN"
GTFS_END="$((SIM_BEGIN + ${GTFS_IMPORT_LOOKAHEAD:-3600}))"
BUS_VTYPES="$SCENARIO_DIR/generated/gtfs-vtypes.add.xml"
BUS_IMPORT_LOG="$SCENARIO_DIR/gtfs-import.log"
BUS_GTFS_SOURCE="$SCENARIO_DIR/guimabus_gtfs.zip"
BUS_GTFS_FILE="$SCENARIO_DIR/generated/guimabus_gtfs_sumo.zip"

"$PYTHON_BIN" "$ROOT_DIR/scripts/normalize_gtfs.py" \
    --input "$BUS_GTFS_SOURCE" --output "$BUS_GTFS_FILE"

import_buses() {
    "$PYTHON_BIN" "$GTFS_TOOLS_DIR/gtfs2pt.py" \
        --region guimaraes --gtfs "$BUS_GTFS_FILE" \
        --date "$GTFS_DATE" --begin "$GTFS_BEGIN" --end "$GTFS_END" \
        --bbox="$MAP_BBOX" --modes bus --network "$SCENARIO_DIR/guimaraes.net.xml" \
        --route-output "$SCENARIO_DIR/buses.rou.xml" \
        --additional-output "$SCENARIO_DIR/buses.add.xml" \
        --vtype-output "$BUS_VTYPES" --stops "$SCENARIO_DIR/ptstops.xml" \
        --duration 10 --bus-stop-length 13 --radius 150 --sort \
        --fcd "$SCENARIO_DIR/generated/fcd" \
        --gpsdat "$SCENARIO_DIR/generated/gpsdat" \
        --network-split "$SCENARIO_DIR/generated/network-split" \
        --map-output "$SCENARIO_DIR/generated/map" \
        --verbose
}

build_direct_buses() {
    "$PYTHON_BIN" "$ROOT_DIR/scripts/build_gtfs_bus_routes.py" \
        --network "$SCENARIO_DIR/guimaraes.net.xml" --gtfs "$BUS_GTFS_FILE" \
        --routes "$SCENARIO_DIR/buses.rou.xml" --stops "$SCENARIO_DIR/buses.add.xml" \
        --date "$GTFS_DATE" --begin "$GTFS_BEGIN" --end "$SIM_END" \
        --max-stop-distance "${BUS_STOP_MAX_DISTANCE:-350}" --seed 42
}

if ! import_buses >"$BUS_IMPORT_LOG" 2>&1; then
    printf 'gtfs2pt.py failed; mapping scheduled trips directly onto bus lanes.\n'
    if ! build_direct_buses >>"$BUS_IMPORT_LOG" 2>&1; then
        printf 'Direct GTFS mapping failed; retrying with the valid demo feed.\n'
        BUS_GTFS_SOURCE="$SCENARIO_DIR/guimabus_gtfs_demo.zip"
        "$PYTHON_BIN" "$ROOT_DIR/scripts/fetch_gtfs.py" --demo --output "$BUS_GTFS_SOURCE"
        BUS_GTFS_FILE="$SCENARIO_DIR/generated/guimabus_gtfs_demo_sumo.zip"
        "$PYTHON_BIN" "$ROOT_DIR/scripts/normalize_gtfs.py" \
            --input "$BUS_GTFS_SOURCE" --output "$BUS_GTFS_FILE"
        build_direct_buses >>"$BUS_IMPORT_LOG" 2>&1
    fi
fi

BUS_COUNT=$("$PYTHON_BIN" -c 'import sys, xml.etree.ElementTree as ET; root=ET.parse(sys.argv[1]).getroot(); print(len(root.findall("vehicle"))+len(root.findall("flow")))' "$SCENARIO_DIR/buses.rou.xml")
if [[ "$BUS_COUNT" == "0" ]]; then
    printf 'GTFS feed produced no bus routes; retrying with the valid demo feed.\n'
    BUS_GTFS_SOURCE="$SCENARIO_DIR/guimabus_gtfs_demo.zip"
    "$PYTHON_BIN" "$ROOT_DIR/scripts/fetch_gtfs.py" --demo --output "$BUS_GTFS_SOURCE"
    BUS_GTFS_FILE="$SCENARIO_DIR/generated/guimabus_gtfs_demo_sumo.zip"
    "$PYTHON_BIN" "$ROOT_DIR/scripts/normalize_gtfs.py" \
        --input "$BUS_GTFS_SOURCE" --output "$BUS_GTFS_FILE"
    build_direct_buses >>"$BUS_IMPORT_LOG" 2>&1
fi
printf '%s\n' "$BUS_GTFS_SOURCE" > "$SCENARIO_DIR/guimabus_gtfs_active.txt"

"$PYTHON_BIN" "$SUMO_HOME/tools/randomTrips.py" \
    --net-file "$SCENARIO_DIR/guimaraes.net.xml" \
    -o "$SCENARIO_DIR/cars.trip.xml" \
    --begin "$SIM_BEGIN" --end "$SIM_END" \
    --insertion-rate "$CAR_RATE" --min-distance 1000 \
    --vehicle-class passenger --prefix cars --seed 42 \
    --vtype-output "$SCENARIO_DIR/generated/cars-vtypes.add.xml"

"$DUAROUTER_BINARY" \
    --net-file "$SCENARIO_DIR/guimaraes.net.xml" \
    --route-files "$SCENARIO_DIR/cars.trip.xml" \
    --output-file "$SCENARIO_DIR/cars.rou.xml" \
    --additional-files "$SCENARIO_DIR/generated/cars-vtypes.add.xml" \
    --begin "$SIM_BEGIN" --end "$SIM_END"

"$PYTHON_BIN" "$SUMO_HOME/tools/randomTrips.py" \
    --net-file "$SCENARIO_DIR/guimaraes.net.xml" \
    -o "$SCENARIO_DIR/bikes.trip.xml" \
    --begin "$SIM_BEGIN" --end "$SIM_END" \
    --insertion-rate "$BIKE_RATE" --min-distance 500 \
    --vehicle-class bicycle --prefix bikes --seed 84 \
    --vtype-output "$SCENARIO_DIR/generated/bikes-vtypes.add.xml"

"$DUAROUTER_BINARY" \
    --net-file "$SCENARIO_DIR/guimaraes.net.xml" \
    --route-files "$SCENARIO_DIR/bikes.trip.xml" \
    --output-file "$SCENARIO_DIR/bikes.rou.xml" \
    --additional-files "$SCENARIO_DIR/generated/bikes-vtypes.add.xml" \
    --begin "$SIM_BEGIN" --end "$SIM_END"

"$PYTHON_BIN" "$ROOT_DIR/scripts/prepare_gtfs_routes.py" \
    --routes "$SCENARIO_DIR/buses.rou.xml" "$SCENARIO_DIR/cars.rou.xml" \
        "$SCENARIO_DIR/bikes.rou.xml" \
    --generated-vtypes "$BUS_VTYPES" "$SCENARIO_DIR/generated/cars-vtypes.add.xml" \
        "$SCENARIO_DIR/generated/bikes-vtypes.add.xml" \
    --vtypes "$SCENARIO_DIR/vtypes.xml" --seed 42

"$PYTHON_BIN" - "$SCENARIO_DIR/guimaraes.sumocfg" "$SIM_BEGIN" "$SIM_END" <<'PY'
import sys
import xml.etree.ElementTree as ET

config_path, begin, end = sys.argv[1:]
tree = ET.parse(config_path)
time = tree.getroot().find("time")
time.find("begin").set("value", begin)
time.find("end").set("value", end)
ET.indent(tree, space="    ")
tree.write(config_path, encoding="utf-8", xml_declaration=True)
PY

SUMMARY_XML="$SCENARIO_DIR/validation-summary.xml"
if ! "$SUMO_BINARY" -c "$SCENARIO_DIR/guimaraes.sumocfg" \
    --summary-output "$SUMMARY_XML" --no-step-log true --duration-log.disable true \
    >"$SCENARIO_DIR/sumo-validation.stdout.log" 2>"$SCENARIO_DIR/sumo-validation.stderr.log"; then
    cat "$SCENARIO_DIR/sumo-validation.stderr.log" >&2
    exit 1
fi

"$PYTHON_BIN" "$ROOT_DIR/scripts/summarize_guimaraes.py" \
    --directory "$SCENARIO_DIR" --summary-xml "$SUMMARY_XML"

"$PYTHON_BIN" "$ROOT_DIR/scripts/export_ns3_trace.py" \
    --config "$SCENARIO_DIR/guimaraes.sumocfg" --sumo-binary "$SUMO_BINARY" \
    --output-prefix "$SCENARIO_DIR/ns3-mobility"