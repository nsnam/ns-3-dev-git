#!/bin/bash
# Generate test coordinates and reference conversion coordinates for the
# GeocentricTopocentricConversionTest.
# This script requires PROJ binaries, wchih can be installed by following
# the official instructions:
# https://proj.org/en/9.4/install.html#install

# Configure printf to use dot-separated integer and decimal parts
export LC_NUMERIC="en_US.UTF-8"
# Coordinates to convert (long (degree), lat (degree), alt (meters)
long_vec=(2.1295495 0.0 30.0 60.0 90.0 120.0 150.0 180.0)
lat_vec=(53.809394 0.0 20.0 40.0 60.0 80.0)
alt_vec=(73.0 500.0 1000.0)
# Reference coordinates (long (degree), lat (degree), alt (meters).
# In particular, the following ones are used as an example in
# Sec. 4.1.3, "IOGP Publication 373-7-2 – Geomatics Guidance Note number 7, part
#  2 – September 2019".
LONG_0=5
LAT_0=55
ALT_0=200
# Reference elipsoids
ellps_vec=(sphere GRS80 WGS84)
# Convert geocentric -> topocentric coordinates
# This conversion corresponds to EPSG Dataset coordinate operation
# method code 9837, see "IOGP Publication 373-7-2 – Geomatics Guidance Note number 7, part
# 2 – September 2019".
for ellps in ${ellps_vec[@]}; do
  echo $ellps
  proj_out=""
  test_coord=""
  for long in ${long_vec[@]}; do
    for lat in ${lat_vec[@]}; do
      for alt in ${alt_vec[@]}; do
        # Coordinates conversion using PROJ
        proj_coord=$(echo $long $lat $alt | cct -d 3 +proj=pipeline +ellps=$ellps +step +proj=cart +step +proj=topocentric +lon_0=$LONG_0 +lat_0=$LAT_0 +h_0=$ALT_0)
        # Update test coordinates string
        test_coord="$test_coord""$(printf "{%f, %f, %f}, " $lat $long $alt)"
        # Update reference conversion coordinates string
        proj_out="$proj_out""$(echo $proj_coord | awk '{print "{" $1 "," $2 "," $3 "},"}')"
      done
    done
  done
  # Output test coordinates and reference conversion coordinates as C++ vector of vectors
  echo "test coordinates"
  echo $test_coord
  echo "converted points"
  echo $proj_out
done
