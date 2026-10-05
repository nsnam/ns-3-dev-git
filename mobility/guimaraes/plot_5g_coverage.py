import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.collections import LineCollection
import xml.etree.ElementTree as ET
import math

print("Loading gNB and MEC positions...")
with open('mobility/guimaraes/gnb_positions.json') as f:
    gnb_data = json.load(f)
gnbs = gnb_data['gnbs']

with open('mobility/guimaraes/mec_topology.json') as f:
    mec_data = json.load(f)
mecs = mec_data['mecs']

print("Parsing SUMO network for street map...")
tree = ET.parse('mobility/guimaraes/guimaraes.net.xml')
root = tree.getroot()
lines = []
for edge in root.findall('edge'):
    if edge.attrib.get('function') == 'internal':
        continue
    for lane in edge.findall('lane'):
        shape = lane.attrib.get('shape')
        if shape:
            points = [tuple(map(float, pt.split(','))) for pt in shape.split()]
            lines.append(points)

print("Calculating 5G coverage...")
xmin, xmax = 0, 7500
ymin, ymax = 0, 11000
grid_res = 50 

x = np.arange(xmin, xmax, grid_res)
y = np.arange(ymin, ymax, grid_res)
X, Y = np.meshgrid(x, y)

fc = 3.5
tx_power = 43
tx_gain = 8
rx_gain = 0

max_rsrp = np.full(X.shape, -200.0)

for gnb in gnbs:
    gnb_x = gnb['x']
    gnb_y = gnb['y']
    gnb_z = gnb['z']
    
    d2 = (X - gnb_x)**2 + (Y - gnb_y)**2
    d3d = np.sqrt(d2 + (gnb_z - 1.5)**2)
    d3d = np.maximum(d3d, 10.0) 
    
    pl = 13.54 + 39.08 * np.log10(d3d) + 20 * np.log10(fc)
    rsrp = tx_power + tx_gain + rx_gain - pl
    max_rsrp = np.maximum(max_rsrp, rsrp)

min_rsrp = -130
max_rsrp[max_rsrp < min_rsrp] = min_rsrp

print("Plotting...")
plt.figure(figsize=(10, 14), facecolor='white')
ax = plt.gca()

lc = LineCollection(lines, colors='black', linewidths=0.6, alpha=0.5)
ax.add_collection(lc)

colors = [(0, 'darkred'), (0.2, 'red'), (0.5, 'yellow'), (0.8, 'lime'), (1, 'darkgreen')]
cm = LinearSegmentedColormap.from_list('signal', colors)
contour = plt.contourf(X, Y, max_rsrp, levels=40, cmap=cm, vmin=-125, vmax=-75, alpha=0.45)
cbar = plt.colorbar(contour, label='RSRP (dBm)', fraction=0.046, pad=0.04)

# Map gNBs to MECs
mec_colors = ['cyan', 'magenta', 'orange', 'purple']
mec_mapping = {}

for gnb in gnbs:
    gnb_id = str(gnb['cell_id'])
    assigned_mec = None
    
    # 1. Try explicit mapping
    for i, mec in enumerate(mecs):
        if gnb_id in mec.get('primary_gnb_cell_ids', []):
            assigned_mec = (mec, mec_colors[i % len(mec_colors)])
            break
            
    # 2. Distance based mapping fallback
    if not assigned_mec:
        min_dist = float('inf')
        for i, mec in enumerate(mecs):
            dist = math.hypot(gnb['x'] - mec['position']['x'], gnb['y'] - mec['position']['y'])
            if dist < min_dist:
                min_dist = dist
                assigned_mec = (mec, mec_colors[i % len(mec_colors)])
    
    mec_mapping[gnb_id] = assigned_mec

# Draw dashed lines from gNBs to their assigned MEC host
for gnb in gnbs:
    gnb_id = str(gnb['cell_id'])
    mec, color = mec_mapping[gnb_id]
    plt.plot([gnb['x'], mec['position']['x']], [gnb['y'], mec['position']['y']], 
             linestyle='--', color=color, linewidth=1.5, alpha=0.8)

# Plot gNBs
gnb_xs = [g['x'] for g in gnbs]
gnb_ys = [g['y'] for g in gnbs]
plt.scatter(gnb_xs, gnb_ys, color='white', edgecolor='black', s=80, marker='^', zorder=5, label='5G gNBs')

# Plot MEC Hosts
for i, mec in enumerate(mecs):
    mx = mec['position']['x']
    my = mec['position']['y']
    color = mec_colors[i % len(mec_colors)]
    plt.scatter(mx, my, color=color, edgecolor='black', s=250, marker='s', zorder=10, 
                label=f"MEC Host: {mec['id']}")
    # Label them
    plt.text(mx + 100, my + 100, mec['id'], fontsize=12, fontweight='bold', 
             bbox=dict(facecolor='white', alpha=0.7, edgecolor='none'))

plt.title('5G NR UMa Coverage Map with MEC Host Topology')
plt.xlabel('X (m)')
plt.ylabel('Y (m)')
plt.legend(loc='upper right')
plt.xlim(xmin, xmax)
plt.ylim(ymin, ymax)
plt.grid(False)

ax.set_facecolor('#f0f0f0')

artifact_path = "mobility/guimaraes/5g_coverage_map.png"
plt.savefig(artifact_path, dpi=300, bbox_inches='tight')
print(f"Map saved to {artifact_path}")
