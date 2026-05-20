#! /usr/bin/env python3

# Copyright (c) 2024 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

import argparse

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.animation import FuncAnimation


def main():
    parser = argparse.ArgumentParser(
        description="Generate an animated MP4 video of satellite orbital traces from CSV data. "
        "The script reads satellite position data over time and creates a 3D animation "
        "showing Earth as a wireframe sphere with satellite positions as red dots. "
        "Optionally (-a)includes antenna orientation vectors as orange lines.",
        epilog="Examples:\n"
        "  python plot-orbital-traces.py -i traces.csv -o animation.mp4\n"
        "  python plot-orbital-traces.py -i traces.csv -o animation.mp4 -a\n"
        "\n"
        "Input CSV format:\n"
        "  Time(ns),Satellite,x,y,z[,x_2,y_2,z_2]\n"
        "  1000000000,1,6371000,0,0[,6372000,0,0]\n"
        "\n"
        "Requires: pandas, matplotlib, numpy, ffmpeg",
    )

    parser.add_argument(
        "--input",
        "-i",
        help="Path to the input CSV file containing satellite trace data. "
        "Expected columns: Time (with 'ns' suffix), Satellite, x, y, z. "
        "For antenna traces: also x_2, y_2, z_2 columns.",
        required=True,
    )
    parser.add_argument(
        "--output",
        "-o",
        help="Path for the output MP4 video file (e.g., 'orbital_animation.mp4'). "
        "Requires ffmpeg to be installed.",
        required=True,
    )
    parser.add_argument(
        "--antenna-trace",
        "-a",
        help="Include antenna orientation vectors in the animation. "
        "Requires x_2, y_2, z_2 columns in the input CSV for each satellite.",
        action="store_true",
    )
    parser.add_argument(
        "--downsample",
        "-d",
        type=int,
        default=1,
        help="Render every Nth frame (default: 1). "
        "Use to speed up animation for large traces (e.g., -d 2 halves frame count).",
    )

    args = parser.parse_args()

    df = pd.read_csv(args.input)
    # Convert from ns to s (rstrip 'ns' to handle both 'ns' suffix and plain numbers)
    df["Time"] = df["Time"].astype(str).str.rstrip("ns").astype(float) * 1e-9

    # Get unique timestamps
    times = np.sort(df["Time"].unique())

    # Pre-group data by time for O(1) per-frame lookup instead of O(n) filtering
    groups = df.groupby("Time")
    # Build list of (x_arr, y_arr, z_arr) tuples indexed by frame number
    frame_data = [
        (
            groups.get_group(t)["x"].values,
            groups.get_group(t)["y"].values,
            groups.get_group(t)["z"].values,
        )
        for t in times
    ]

    # Pre-group antenna data per frame
    if args.antenna_trace:
        frame_antenna = {}
        for t in times:
            g = groups.get_group(t)
            frame_antenna[t] = {}
            for sat in g["Satellite"].unique():
                sg = g[g["Satellite"] == sat]
                frame_antenna[t][sat] = (
                    sg[["x", "y", "z"]].values[0],
                    sg[["x_2", "y_2", "z_2"]].values[0],
                )

    # Set up the 3D figure
    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")

    # Plot Earth as a sphere
    radius = 6371e3
    u, v = np.mgrid[0 : 2 * np.pi : 50j, 0 : np.pi : 25j]
    xs = radius * np.cos(u) * np.sin(v)
    ys = radius * np.sin(u) * np.sin(v)
    zs = radius * np.cos(v)
    ax.plot_wireframe(xs, ys, zs, color="lightblue", linewidth=0.6, alpha=0.7)

    # Initialize scatter
    scatter = ax.scatter([], [], [], color="red", s=5)

    # Initialize line objects for each satellite
    if args.antenna_trace:
        lines = {}
        for sat in df["Satellite"].unique():
            (line,) = ax.plot([], [], [], alpha=0.7, lw=0.7, color="orange")
            lines[sat] = line
        all_sats = list(df["Satellite"].unique())

    # Downsample frames for speed: render every Nth frame
    step = max(1, args.downsample)

    def make_update(frame_step, times, frame_data, frame_antenna):
        def update(frame):
            i = frame * frame_step
            current_time = times[i]
            x, y, z = frame_data[i]
            scatter._offsets3d = (x, y, z)
            if args.antenna_trace:
                for sat, line in zip(all_sats, lines.values()):
                    if sat in frame_antenna.get(current_time, {}):
                        pos, ant = frame_antenna[current_time][sat]
                        line.set_data_3d([pos[0], ant[0]], [pos[1], ant[1]], [pos[2], ant[2]])
                    else:
                        line.set_data_3d([], [], [])
            ax.set_title(f"Time = {current_time:.2f} s")
            if args.antenna_trace:
                return scatter, *lines.values()
            else:
                return scatter

        return update

    ani = FuncAnimation(
        fig,
        make_update(step, times, frame_data, frame_antenna),
        frames=len(times) // step,
        interval=100,
        blit=False,
    )
    ani.save(args.output, writer="ffmpeg", fps=30)


if __name__ == "__main__":
    main()
