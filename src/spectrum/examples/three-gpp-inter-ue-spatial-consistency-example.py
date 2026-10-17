# Copyright (c) 2026, Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

"""Run and plot the three-gpp-inter-ue-spatial-consistency-example.

Runs the example twice per comparison, with the compared feature disabled and
enabled, and produces one figure per comparison showing:
  - the two SNR maps,
  - their small-scale residuals (gain minus a local box average), which
    isolate the shadow-fading term from the distance-dependent path loss,
  - the gain CDFs of the two maps.

Two comparisons are available (both run by default):
  - spatialConsistency: inter-UE spatially consistent generation (TR 38.901
    Sec. 7.6.3.1). Spatial consistency changes the spatial structure of the
    shadow fading (from per-point white noise to a correlated field), while
    leaving its marginal distribution untouched: the two CDFs are expected to
    coincide, up to the sampling noise of a single drop (a map spanning a few
    correlation distances only contains a few tens of independent
    shadow-fading samples).
  - largeBandwidth: intra-cluster angular and delay spread modeling (TR
    38.901 Sec. 7.6.2.2). Each ray becomes an individually delayed tap with
    unequal powers, changing the fast-fading realization of every point but
    preserving the per-cluster power budget: the SNR maps differ point by
    point in their fast-fading term, while the CDFs are again expected to
    coincide.

Usage (from the ns-3 root directory, after building the examples):
  python3 src/spectrum/examples/three-gpp-inter-ue-spatial-consistency-example.py
"""

import argparse
import subprocess
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.gridspec import GridSpec, GridSpecFromSubplotSpec

EXAMPLE = "three-gpp-inter-ue-spatial-consistency-example"
COMPARISONS = {
    "spatialConsistency": {
        "runs": [
            ("scoff", "false", "spatial consistency OFF", "#c0392b"),
            ("scon", "true", "spatial consistency ON", "#2471a3"),
        ],
        "title": "inter-UE spatial consistency (TR 38.901 Sec. 7.6.3.1)",
        "suffix": "",
    },
    "largeBandwidth": {
        "runs": [
            ("lboff", "false", "large bandwidth modeling OFF", "#c0392b"),
            ("lbon", "true", "large bandwidth modeling ON", "#2471a3"),
        ],
        "title": "large bandwidth modeling (TR 38.901 Sec. 7.6.2.2)",
        "suffix": "-large-bandwidth",
    },
}


def run_simulations(args, compare):
    """Run the example twice (compared feature off/on) via the ns3 script."""
    for tag, enabled, label, _ in COMPARISONS[compare]["runs"]:
        cmdline = (
            f"{EXAMPLE} --scenario={args.scenario} --frequency={args.frequency}"
            f" --condition={args.condition}"
            f" --xRes={args.res} --yRes={args.res}"
            f" --{compare}={enabled} --simTag={file_tag(args, tag)}"
        )
        print(f"Running: {cmdline}")
        subprocess.run([sys.executable, args.ns3, "run", cmdline], check=True)


def file_tag(args, tag):
    """Build the simTag of one run, including the channel condition."""
    return tag if args.condition == "Probabilistic" else f"{args.condition.lower()}_{tag}"


def load(args, tag):
    """Load one output file as (xs, ys, snr-grid)."""
    m = np.loadtxt(f"three-gpp-inter-ue-spatial-consistency-{file_tag(args, tag)}.out")
    xs = np.unique(m[:, 0])
    ys = np.unique(m[:, 1])
    gain = m[:, 2].reshape(len(xs), len(ys)).T
    return xs, ys, gain


def box_smooth(z, r=3):
    """Local box average of a 2D grid with radius r (edge-padded)."""
    zp = np.pad(z, r, mode="edge")
    k = 2 * r + 1
    c = np.cumsum(np.cumsum(zp, 0), 1)
    c = np.pad(c, ((1, 0), (1, 0)))
    return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)


def plot(args, compare, limits=None):
    runs = COMPARISONS[compare]["runs"]
    xs, ys, off = load(args, runs[0][0])
    _, _, on = load(args, runs[1][0])
    extent = [xs.min(), xs.max(), ys.min(), ys.max()]

    res_off = off - box_smooth(off)
    res_on = on - box_smooth(on)

    if limits is None:
        vmin = min(off.min(), on.min())
        vmax = max(off.max(), on.max())
        rlim = np.percentile(np.abs(np.concatenate([res_off.ravel(), res_on.ravel()])), 99)
    else:
        vmin, vmax, rlim = limits

    fig = plt.figure(figsize=(17, 9), constrained_layout=True)
    gs = GridSpec(2, 3, figure=fig, width_ratios=[1.0, 1.0, 1.15])

    for row, (gain, res, (_, _, label, _)) in enumerate(
        [(off, res_off, runs[0]), (on, res_on, runs[1])]
    ):
        ax_gain = fig.add_subplot(gs[row, 0])
        im_gain = ax_gain.imshow(
            gain,
            origin="lower",
            extent=extent,
            cmap="viridis",
            vmin=vmin,
            vmax=vmax,
            aspect="equal",
        )
        ax_gain.set_title(f"{label} - SNR (std {gain.std():.2f} dB)")
        fig.colorbar(im_gain, ax=ax_gain, shrink=0.85, label="SNR (dB)")

        ax_res = fig.add_subplot(gs[row, 1])
        im_res = ax_res.imshow(
            res, origin="lower", extent=extent, cmap="RdBu_r", vmin=-rlim, vmax=rlim, aspect="equal"
        )
        ax_res.set_title(f"{label} - residual (std {res.std():.2f} dB)")
        fig.colorbar(im_res, ax=ax_res, shrink=0.85, label="SNR - local mean (dB)")

        for ax in (ax_gain, ax_res):
            ax.set_xlabel("x (m)")
            ax.set_ylabel("y (m)")

    # third column, vertically centered: gain CDF of the two maps
    sub = GridSpecFromSubplotSpec(3, 1, subplot_spec=gs[:, 2], height_ratios=[0.5, 1.0, 0.5])
    ax_cdf = fig.add_subplot(sub[1, 0])
    for gain, (_, _, label, color) in [(off, runs[0]), (on, runs[1])]:
        v = np.sort(gain.ravel())
        cdf = np.arange(1, v.size + 1) / v.size
        ax_cdf.plot(v, cdf, lw=2.2, color=color, label=label)
    ax_cdf.set_xlabel("SNR (dB)")
    ax_cdf.set_ylabel("CDF")
    ax_cdf.set_title("SNR CDF")
    ax_cdf.grid(True, alpha=0.3)
    ax_cdf.set_ylim(0, 1)
    ax_cdf.set_xlim(vmin, vmax)
    ax_cdf.legend(loc="lower right")

    fig.suptitle(
        f"{args.scenario} SNR map ({args.res}x{args.res}, {args.condition} condition) - "
        + COMPARISONS[compare]["title"],
        fontsize=14,
        fontweight="bold",
    )

    suffix = COMPARISONS[compare]["suffix"]
    out = f"three-gpp-inter-ue-spatial-consistency{suffix}.png"
    if args.condition != "Probabilistic":
        out = f"three-gpp-inter-ue-spatial-consistency-{args.condition.lower()}{suffix}.png"
    fig.savefig(out, dpi=130)
    print(f"saved {out}")
    print(f"OFF: SNR std {off.std():.2f} dB, residual std {res_off.std():.3f} dB")
    print(f"ON : SNR std {on.std():.2f} dB, residual std {res_on.std():.3f} dB")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--ns3", default="./ns3", help="path to the ns3 script")
    parser.add_argument(
        "--scenario", default="UMa", choices=["UMa", "UMi", "RMa"], help="3GPP propagation scenario"
    )
    parser.add_argument(
        "--condition",
        default="all",
        choices=["all", "Probabilistic", "LOS", "NLOS"],
        help="channel condition of the map (default: run all three)",
    )
    parser.add_argument("--frequency", type=float, default=3.5e9, help="operating frequency in Hz")
    parser.add_argument("--res", type=int, default=200, help="grid resolution per axis")
    parser.add_argument(
        "--compare",
        default="all",
        choices=["all", *COMPARISONS.keys()],
        help="feature to compare off/on (default: run both comparisons)",
    )
    parser.add_argument(
        "--skip-run", action="store_true", help="only plot, reusing existing output files"
    )
    args = parser.parse_args()

    conditions = ["Probabilistic", "LOS", "NLOS"] if args.condition == "all" else [args.condition]
    compares = list(COMPARISONS.keys()) if args.compare == "all" else [args.compare]
    for compare in compares:
        for condition in conditions:
            args.condition = condition
            if not args.skip_run:
                run_simulations(args, compare)

        # shared color limits across all conditions, so the figures are
        # immediately comparable
        grids = []
        for condition in conditions:
            args.condition = condition
            grids += [load(args, tag)[2] for tag, _, _, _ in COMPARISONS[compare]["runs"]]
        vmin = min(g.min() for g in grids)
        vmax = max(g.max() for g in grids)
        rlim = np.percentile(
            np.abs(np.concatenate([(g - box_smooth(g)).ravel() for g in grids])), 99
        )

        for condition in conditions:
            args.condition = condition
            plot(args, compare, (vmin, vmax, rlim))


if __name__ == "__main__":
    main()
