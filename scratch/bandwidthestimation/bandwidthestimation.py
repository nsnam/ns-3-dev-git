import argparse
import os
import matplotlib.pyplot as plt

# Plot the Estimated BW over Time
import numpy as np

estimatedBWTraceTimes = []
estimatedBWTraceVals = []

dir = 'outputs'

with open(os.path.join(dir, 'estimated-bw.tr'),'r') as f:
    for line in f:
        estimatedBWTrace = line.split()

        estimatedBWTraceTimes.append(float(estimatedBWTrace[0]))
        estimatedBWTraceVals.append((8 * float(estimatedBWTrace[1])) / (1000 * 1000))

estimatedBWFileName = os.path.join(dir, 'estimated-bw.png')

print(np.average(estimatedBWTraceVals))

plt.figure()
plt.plot(estimatedBWTraceTimes, estimatedBWTraceVals)
plt.ylabel('Bandwidth')
plt.xlabel('Seconds')
plt.grid()
plt.savefig(estimatedBWFileName)
print('Saving ' + estimatedBWFileName)