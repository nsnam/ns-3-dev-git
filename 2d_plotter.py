#! /usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys

type = str(sys.argv[1])

def plot(i):
    path = str(sys.argv[i])
    print(path)
    file = open(path)
    array = np.loadtxt(file, delimiter=',')
    if type == "scatter" or type == "s": plt.scatter(array[:, 0], array[:, 1], label=path)
    elif type == "line" or type == "l" : plt.plot(array[:, 0], array[:, 1], label=path)

for i in range(2, sys.argv.__len__()):
    plot(i)

plt.locator_params(axis='y', nbins=15)
plt.grid()
plt.legend(loc='upper left')
plt.show()