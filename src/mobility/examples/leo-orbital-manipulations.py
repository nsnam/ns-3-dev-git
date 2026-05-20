#!/usr/bin/env python3
"""
LEO Orbital Manipulations Visualization Script

@file
@ingroup leo

This script demonstrates how different orbital parameters affect the shape and orientation
of Low Earth Orbit (LEO) satellite trajectories. It generates 3D visualizations showing
the effects of various parameters on orbital planes and satellite positions over time.

The script mirrors the orbital mechanics implemented in the ns-3 LeoCircularOrbitMobilityModel,
providing a visual understanding of how each parameter influences satellite motion.

USAGE:
    python leo-orbital-manipulations.py

    The script will display a 3D matplotlib plot showing:
    - Satellite positions sampled over one orbital period (blue dots)
    - Starting position (green dot)
    - Ending position (red dot)
    - Earth reference circle (black dashed line)

PARAMETER EFFECTS:

1. orbit_alt_km (Orbit Altitude):
   - Controls the distance from Earth's center
   - Higher altitude = larger orbit radius = slower orbital speed
   - Affects orbital period (T ∝ r^(3/2))

2. time_step_sec (Time Resolution):
   - Controls sampling frequency of orbital positions
   - Smaller values = more position samples = smoother orbit visualization
   - Larger values = fewer samples = coarser orbit representation

3. inclination_deg (Orbital Inclination):
   - Angle between orbital plane and Earth's equatorial plane
   - 0° = equatorial orbit (over equator)
   - 90° = polar orbit (over poles)
   - >90° = retrograde orbit (opposite direction)

4. raan_deg (Right Ascension of Ascending Node):
   - Longitude where the satellite crosses the equator going north
   - Rotates the entire orbital plane around Earth's Z-axis
   - 0° = orbital plane aligned with prime meridian
   - 90° = orbital plane rotated 90° east

5. plane_rotation_deg (Plane Rotation):
   - Additional rotation of the orbital plane about its normal vector
   - Affects the starting position within the orbital plane
   - Equivalent to rotating the satellite's position along its orbit

GENERATED FIGURES:
This script was used to generate the figures in src/mobility/doc/figures/:
- leo-orbit-res-*.png: Various parameter combinations showing orbital effects

DEPENDENCIES:
- numpy
- matplotlib
- mpl_toolkits.mplot3d

EXAMPLE OUTPUT:
The script produces a 3D plot with:
- Blue dots: Satellite positions over one orbit
- Green dot: Starting position
- Red dot: Ending position
- Black dashed circle: Earth's equatorial radius reference
"""

import argparse

import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 (needed for 3D projection)

KM_TO_M = 1000.0
LEO_EARTH_GGC = 398600.7  # Earth gravitational parameter μ in km^3/s^2

# WGS84 equatorial radius in kilometers (a.k.a. semi-major axis a)
EARTH_RADIUS_KM = 6378.137


def generate_progress_vector(
    orbit_alt_km: float, time_step_sec: float, inc_deg: float = 28.0
) -> np.ndarray:
    """
    Generate an array of angular offsets ("progress vector") that advance around a circular orbit
    by a fixed time step.

    This logic:
      - Assumes a circular orbit with radius r = (Earth radius + altitude).
      - Uses circular-orbit speed v = sqrt(μ / r).
      - Converts time step into arc-length step: step_size = v * dt.
      - Converts arc-length to fraction of orbital circumference, then to an angle increment.

    Parameters
    ----------
    orbit_alt_km
        Orbit altitude above the reference Earth radius, in kilometers.
    time_step_sec
        Simulation sampling interval (dt), in seconds. One progress-vector entry is produced per dt.
    inc_deg
        Inclination in degrees, used only to replicate the sign convention (flip for retrograde
        if inclination > 90 deg).

    Returns
    -------
    np.ndarray
        Array of angles (radians) from 0 up to ~2π (one orbital revolution), sampled at dt.
        The length is computed so that the last step is approximately one full orbit.
    """
    # Orbit radius from Earth's equatorial radius (WGS84) + altitude
    orbit_radius_km = EARTH_RADIUS_KM + orbit_alt_km

    # Circular-orbit speed (km/s): v = sqrt(μ / r)
    node_speed_kms = np.sqrt(LEO_EARTH_GGC / orbit_radius_km)

    # Orbital circumference for a circular orbit: 2πr (km)
    orbit_perimeter_km = orbit_radius_km * 2.0 * np.pi

    # Distance traveled in one simulation step (km)
    step_size_km = node_speed_kms * time_step_sec

    # Fraction of full orbit per step (dimensionless)
    step_fraction = step_size_km / orbit_perimeter_km

    # Number of steps to cover ~one full orbit
    steps = int(round(1.0 / step_fraction))

    # Angle advanced per step (radians), before applying sign
    step_angle_base = 2.0 * np.pi * step_fraction

    # Match C++ sign convention: flip direction for inclinations > 90° (retrograde)
    sign = 1 if np.deg2rad(inc_deg) <= (np.pi / 2.0) else -1
    step_angle = sign * step_angle_base

    # Angles from 0, step_angle, 2*step_angle, ...
    return np.array([k * step_angle for k in range(steps)], dtype=float)


def apply_raan_and_inclination(
    x: np.ndarray, y: np.ndarray, z: np.ndarray, raan_deg: float, inclination_deg: float
):
    """
    Rotate orbit points from the initial XY plane (z=0) into a tilted/oriented orbital plane.

    The transform applied is:
        r_new = Rz(Ω) * Rx(i) * r
    where:
        Ω (RAAN) rotates the line of nodes around the +Z axis,
        i (inclination) tilts the plane by rotating around the +X axis.

    Parameters
    ----------
    x, y, z
        Arrays (N,) representing point coordinates (km).
    raan_deg
        Right ascension of the ascending node Ω, in degrees (rotation about +Z).
    inclination_deg
        Inclination i, in degrees (rotation about +X).

    Returns
    -------
    (x2, y2, z2)
        Rotated coordinate arrays (km).
    """
    Om = np.deg2rad(raan_deg)
    i = np.deg2rad(inclination_deg)

    # Rotation about Z axis (RAAN)
    Rz = np.array(
        [
            [np.cos(Om), -np.sin(Om), 0.0],
            [np.sin(Om), np.cos(Om), 0.0],
            [0.0, 0.0, 1.0],
        ]
    )

    # Rotation about X axis (inclination tilt)
    Rx = np.array(
        [
            [1.0, 0.0, 0.0],
            [0.0, np.cos(i), -np.sin(i)],
            [0.0, np.sin(i), np.cos(i)],
        ]
    )

    pts = np.vstack([x, y, z])  # shape (3, N)
    pts_new = (Rz @ Rx) @ pts  # shape (3, N)
    return pts_new[0], pts_new[1], pts_new[2]


def rodrigues_rotate_points(
    x: np.ndarray,
    y: np.ndarray,
    z: np.ndarray,
    inclination_deg: float,
    raan_deg: float,
    plane_rotation_deg: float,
):
    """
    Apply an additional rigid rotation to all points using Rodrigues' rotation formula.

    This rotates each 3D point about a user-defined axis vector `n` by `plane_rotation_deg`.
    Rodrigues’ formula (for unit axis n) is: v' = v*cos(a) + (n×v)*sin(a) + n*(n·v)*(1-cos(a)).

    Notes
    -----
    - The axis `n` you compute below is treated as the rotation axis in the same coordinate frame
      as (x,y,z). Ensure the axis definition matches the physical meaning you want.
    - `n` is normalized internally; it must not be the zero vector.

    Parameters
    ----------
    x, y, z
        Arrays (N,) of point coordinates (km).
    inclination_deg
        Inclination used in the axis construction below (degrees).
    raan_deg
        RAAN used in the axis construction below (degrees).
    plane_rotation_deg
        Rotation angle about axis n, in degrees.

    Returns
    -------
    (x2, y2, z2)
        Rotated coordinate arrays (km).
    """
    P = np.column_stack([x, y, z]).astype(float)  # shape (N,3)

    inc = np.deg2rad(inclination_deg)
    Om = np.deg2rad(raan_deg)
    a = np.deg2rad(plane_rotation_deg)

    # Rotate the plane axis (derived from RAAN).
    n = np.array([np.sin(Om) * np.sin(inc), -np.cos(Om) * np.sin(inc), np.cos(inc)], dtype=float)

    n_norm = np.linalg.norm(n)
    if n_norm == 0.0:
        raise ValueError("Rotation axis n is zero; check inclination/raan inputs.")
    n = n / n_norm  # must be unit axis for Rodrigues

    c = np.cos(a)
    s = np.sin(a)

    # Vectorized Rodrigues rotation for all points:
    # P2 = P*c + (n×P)*s + n*(P·n)*(1-c)
    P2 = P * c + np.cross(n, P) * s + (n * (P @ n)[:, None]) * (1.0 - c)

    return P2[:, 0], P2[:, 1], P2[:, 2]


# ----------------------------
# Example parameters - Modify these to see different orbital effects
# ----------------------------


def main(
    orbit_alt_km: float = 500.0,
    time_step_sec: float = 600.0,
    inclination_deg: float = 30.0,
    raan_deg: float = 0.0,
    plane_rotation_deg: float = 60.0,
):
    # ----------------------------
    # Generate and plot the orbital trajectory
    # ----------------------------

    # 1) Generate angular progress over ~one orbit based on orbital mechanics
    angles = generate_progress_vector(orbit_alt_km, time_step_sec, inc_deg=inclination_deg)

    # 2) Create initial circular orbit in XY plane (equatorial)
    r_km = EARTH_RADIUS_KM + orbit_alt_km
    x = r_km * np.cos(angles)
    y = r_km * np.sin(angles)
    z = np.zeros_like(angles)

    # 3) Apply orbital plane orientation (RAAN + inclination)
    x, y, z = apply_raan_and_inclination(
        x, y, z, raan_deg=raan_deg, inclination_deg=inclination_deg
    )

    # 4) Apply additional rotation within the orbital plane
    x, y, z = rodrigues_rotate_points(
        x,
        y,
        z,
        inclination_deg=inclination_deg,
        raan_deg=raan_deg,
        plane_rotation_deg=plane_rotation_deg,
    )

    # ----------------------------
    # Create 3D visualization
    # ----------------------------

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection="3d")

    # Plot satellite positions over the orbit
    ax.scatter(x, y, z, color="tab:blue", s=8, label="Satellite positions")
    ax.scatter(x[0], y[0], z[0], color="green", s=50, label="Starting position")
    ax.scatter(x[-1], y[-1], z[-1], color="red", s=50, label="Ending position")

    # Add Earth reference circle in equatorial plane
    theta = np.linspace(0.0, 2.0 * np.pi, 200)
    ax.plot(
        EARTH_RADIUS_KM * np.cos(theta),
        EARTH_RADIUS_KM * np.sin(theta),
        np.zeros_like(theta),
        "k--",
        linewidth=1.0,
        label="Earth's equator",
    )

    # Configure plot appearance
    ax.set_xlabel("X (km)")
    ax.set_ylabel("Y (km)")
    ax.set_zlabel("Z (km)")
    ax.set_title(
        "LEO Satellite Orbit Visualization\n"
        f"Altitude: {orbit_alt_km} km | Time step: {time_step_sec} s | Samples: {len(angles)}\n"
        f"Inclination: {inclination_deg}° | RAAN: {raan_deg}° | Plane rotation: {plane_rotation_deg}°"
    )
    ax.legend()
    ax.grid(True, alpha=0.3)

    # Set equal aspect ratio for proper 3D visualization
    max_range = np.array([x.max() - x.min(), y.max() - y.min(), z.max() - z.min()]).max() / 2.0
    mid_x = (x.max() + x.min()) * 0.5
    mid_y = (y.max() + y.min()) * 0.5
    mid_z = (z.max() + z.min()) * 0.5
    ax.set_xlim(mid_x - max_range, mid_x + max_range)
    ax.set_ylim(mid_y - max_range, mid_y + max_range)
    ax.set_zlim(mid_z - max_range, mid_z + max_range)

    plt.tight_layout()
    fig_name = f"leo-orbit-res-{time_step_sec}s-inc-{inclination_deg}deg-raan-{raan_deg}deg-planeRot-{plane_rotation_deg}deg.png"
    plt.savefig(fig_name, dpi=300)
    # plt.show()
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="LEO Orbital Manipulations Visualization Script\n\n"
        "This script demonstrates how different orbital parameters affect the shape and orientation "
        "of Low Earth Orbit (LEO) satellite trajectories. It generates 3D visualizations showing "
        "the effects of various parameters on orbital planes and satellite positions over time.\n\n"
        "The script mirrors the orbital mechanics implemented in the ns-3 LeoCircularOrbitMobilityModel, "
        "providing a visual understanding of how each parameter influences satellite motion.\n\n"
        "Output: 3D matplotlib plot showing satellite positions over one orbital period, "
        "with Earth reference circle and start/end position markers.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--orbit-alt-km", type=float, default=500.0, help="Altitude above Earth's surface (km)"
    )
    parser.add_argument(
        "--time-step-sec", type=float, default=600.0, help="Time between position samples (seconds)"
    )
    parser.add_argument(
        "--inclination-deg", type=float, default=30.0, help="Angle from equatorial plane (degrees)"
    )
    parser.add_argument(
        "--raan-deg", type=float, default=0.0, help="Longitude of ascending node (degrees)"
    )
    parser.add_argument(
        "--plane-rotation-deg",
        type=float,
        default=60.0,
        help="Additional rotation within plane (degrees)",
    )

    args = parser.parse_args()

    # Call main with parsed arguments
    main(
        args.orbit_alt_km,
        args.time_step_sec,
        args.inclination_deg,
        args.raan_deg,
        args.plane_rotation_deg,
    )
