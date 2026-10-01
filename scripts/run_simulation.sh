#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCENARIO_DIR="$ROOT_DIR/mobility/guimaraes"
OUTPUT_DIR="$ROOT_DIR/results/raw_data"
SUMO_BINARY="${SUMO_BINARY:-sumo}"
PYTHON_BIN="${PYTHON:-$ROOT_DIR/../bin/python}"
if [[ ! -x "$PYTHON_BIN" ]]; then
    PYTHON_BIN="$(command -v python3)"
fi

if [[ -n "${OPENCELLID_INPUT:-}" ]]; then
    "$PYTHON_BIN" "$ROOT_DIR/scripts/parse_opencellid_gnb.py" \
        --input "$OPENCELLID_INPUT" --network "$SCENARIO_DIR/guimaraes.net.xml" \
        --output "$SCENARIO_DIR/gnb_positions.json"
elif [[ -s "$SCENARIO_DIR/gnb_positions.json" && "${FORCE_GNB_REFRESH:-0}" != "1" ]]; then
    printf 'Reusing cached OpenCellID positions: %s\n' "$SCENARIO_DIR/gnb_positions.json"
elif [[ -n "${OPENCELLID_API_KEY:-}" ]]; then
    "$PYTHON_BIN" "$ROOT_DIR/scripts/parse_opencellid_gnb.py" \
        --network "$SCENARIO_DIR/guimaraes.net.xml" \
        --output "$SCENARIO_DIR/gnb_positions.json"
else
    printf 'Set OPENCELLID_API_KEY, OPENCELLID_INPUT, or provide mobility/guimaraes/gnb_positions.json.\n' >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
cd "$ROOT_DIR"
./ns3 build

SIM_DURATION="${SIM_DURATION:-120}"
SUMO_START_TIME="${SUMO_START_TIME:-25200}"
MAX_UES=$("$PYTHON_BIN" "$ROOT_DIR/scripts/estimate_ue_pool.py" \
    --config "$SCENARIO_DIR/guimaraes.sumocfg" --sumo-binary "$SUMO_BINARY" \
    --start-time "$SUMO_START_TIME" --duration "$SIM_DURATION" \
    --margin "${UE_POOL_MARGIN:-4}")

./ns3 run "mec-5g-guimaraes --sumoConfig=$SCENARIO_DIR/guimaraes.sumocfg --gnbPositions=$SCENARIO_DIR/gnb_positions.json --outputDirectory=$OUTPUT_DIR --duration=$SIM_DURATION --maxUes=$MAX_UES --sumoStartTime=$SUMO_START_TIME" \
    2>&1 | tee "$OUTPUT_DIR/simulation.log"

"$PYTHON_BIN" "$ROOT_DIR/scripts/convert_nr_traces_to_csv.py" --directory "$OUTPUT_DIR"