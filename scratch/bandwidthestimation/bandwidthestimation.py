import argparse
import os
import matplotlib.pyplot as plt
import numpy as np

# Plot the Estimated BW over Time

parser = argparse.ArgumentParser()
parser.add_argument('--dir', '-d',
                    help="Directory to find the trace files",
                    required=True,
                    action="store",
                    dest="dir")
args = parser.parse_args()

estimatedBWTraceTimes = []
estimatedBWTraceVals = []

dir = 'outputs'

with open(os.path.join(args.dir, 'estimated-bw.tr'),'r') as f:
    for line in f:
        estimatedBWTrace = line.split()

        estimatedBWTraceTimes.append(float(estimatedBWTrace[0]))
        estimatedBWTraceVals.append((8 * float(estimatedBWTrace[1])) / (1000 * 1000))

estimatedBWFileName = os.path.join(args.dir, 'estimated-bw.png')

plt.figure()
plt.plot(estimatedBWTraceTimes, estimatedBWTraceVals, c="C0")
plt.ylabel('Bandwidth')
plt.xlabel('Seconds')
plt.grid()
plt.savefig(estimatedBWFileName)
print('Saving ' + estimatedBWFileName)