import json
import numpy as np

# Load gNBs
with open('mobility/guimaraes/gnb_positions.json') as f:
    gnb_data = json.load(f)
gnbs = gnb_data['gnbs']

pts = np.array([[g['x'], g['y']] for g in gnbs])
gnb_ids = [str(g['cell_id']) for g in gnbs]

# K-Means clustering (k=3)
np.random.seed(42)
idx1 = np.argmin(pts[:, 0])
idx2 = np.argmax(pts[:, 0])
idx3 = np.argmax(pts[:, 1])
centroids = np.array([pts[idx1], pts[idx2], pts[idx3]])

labels = np.zeros(len(pts))
for _ in range(100):
    dists = np.linalg.norm(pts[:, np.newaxis] - centroids, axis=2)
    labels = np.argmin(dists, axis=1)
    
    new_centroids = []
    for i in range(3):
        cluster_pts = pts[labels == i]
        if len(cluster_pts) > 0:
            new_centroids.append(cluster_pts.mean(axis=0))
        else:
            new_centroids.append(centroids[i])
    new_centroids = np.array(new_centroids)
    
    if np.allclose(centroids, new_centroids):
        break
    centroids = new_centroids

# Find Hub gNBs (closest actual gNB to each centroid)
hub_gnbs = []
for c in centroids:
    dists = np.linalg.norm(pts - c, axis=1)
    closest_idx = np.argmin(dists)
    hub_gnbs.append(gnbs[closest_idx])

# Reassign all gNBs to the closest Hub gNB (Hub-and-Spoke Voronoi)
hub_pts = np.array([[g['x'], g['y']] for g in hub_gnbs])
final_labels = np.argmin(np.linalg.norm(pts[:, np.newaxis] - hub_pts, axis=2), axis=1)

# Update MEC Topology
with open('mobility/guimaraes/mec_topology.json') as f:
    mec_data = json.load(f)

for i in range(3):
    if i < len(mec_data['mecs']):
        hub = hub_gnbs[i]
        mec_data['mecs'][i]['position']['x'] = hub['x']
        mec_data['mecs'][i]['position']['y'] = hub['y']
        mec_data['mecs'][i]['position']['z'] = hub['z']
        
        # Set blended backhaul delay
        mec_data['mecs'][i]['backhaul']['delay_ms'] = 1.5
        
        # Assign Spoke gNBs
        cluster_gnbs = [gnb_ids[j] for j in range(len(gnb_ids)) if final_labels[j] == i]
        mec_data['mecs'][i]['primary_gnb_cell_ids'] = cluster_gnbs
        
        print(f"MEC_{i} colocated at Hub gNB {hub['cell_id']} serving {len(cluster_gnbs)} gNBs.")

with open('mobility/guimaraes/mec_topology.json', 'w') as f:
    json.dump(mec_data, f, indent=2)

print("mec_topology.json updated successfully with Hub-and-Spoke model!")
