#! /usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import sys

path = str(sys.argv[1])
path2 = str(sys.argv[2])


file = open(path)
file2 = open(path2)

array = np.loadtxt(file, delimiter=',')
array2 = np.loadtxt(file2, delimiter=',')
# print(array)
plt.scatter(array[:, 0], array[:, 1], c='b', marker='o', label=path)
plt.scatter(array2[:, 0], array2[:, 1], c='r', marker='s', label=path2)
plt.legend(loc='upper left')
plt.show()