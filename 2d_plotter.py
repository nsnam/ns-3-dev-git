#! /usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys

def plot(i):
    path = str(sys.argv[i])
    print(path)
    file = open(path)
    array = np.loadtxt(file, delimiter=',')
    plt.scatter(array[:, 0], array[:, 1], label=path)

for i in range(1, sys.argv.__len__()):
    plot(i)

plt.grid()
plt.legend(loc='upper left')
plt.show()