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
    parser = argparse.ArgumentParser()

    parser.add_argument("--input", "-i", help="Path to the input file containing traces.")
    parser.add_argument("--output", "-o", help="Path for the .mp4 output file.")
    parser.add_argument(
        "--antenna-trace",
        "-a",
        help="Flag to indicate the traces contain a second coordinate to draw Antenna Orientation Vector",
        action="store_true",
    )

    args = parser.parse_args()

    df = pd.read_csv(args.input, sep=":")
    # Convert from ns to s
    df["Time"] = df["Time"].str.replace("ns", "").astype(float) * 1e-9

    # Get unique timestamps
    times = np.sort(df["Time"].unique())

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

    # initialize line objects for each satellite
    if args.antenna_trace:
        lines = {}
        for sat in df["Satellite"].unique():
            # creates empty Line3D objects to be filled later
            (line,) = ax.plot([], [], [], alpha=0.7, lw=0.7, color="orange")
            lines[sat] = line

    # Updates data for each frame
    def update(frame):
        current_time = times[frame]
        data = df[df["Time"] == current_time]
        # update scatter points
        scatter._offsets3d = (data["x"], data["y"], data["z"])
        if args.antenna_trace:
            # update lines between pos and pos2
            for sat, line in lines.items():
                d_sat = data[data["Satellite"] == sat]
                if not d_sat.empty:
                    x, y, z = d_sat[["x", "y", "z"]].values[0]
                    x2, y2, z2 = d_sat[["x_2", "y_2", "z_2"]].values[0]
                    line.set_data_3d([x, x2], [y, y2], [z, z2])
                else:
                    line.set_data_3d([], [], [])
        ax.set_title(f"Time = {current_time:.2f} s")
        if args.antenna_trace:
            return scatter, *lines.values()
        else:
            return scatter

    # animate without blitting (3D + multiple artists)
    ani = FuncAnimation(fig, update, frames=len(times), interval=100, blit=False)
    ani.save(args.output, writer="ffmpeg", fps=30)


if __name__ == "__main__":
    main()
