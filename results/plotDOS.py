#!/usr/bin/env python3
# Script used to plot results from the the test of the DoS attack
# Data is parsed from aggregated results created with ./get_dataDOS.sh
import os
import numpy as np
import matplotlib.pyplot as plt

parsedfolder = os.path.abspath("parsedDOS/")
results = {}

# CHANGE THE FOLDER TO READ THE CORRECT FILES
path = parsedfolder + "/parsedDOS.txt"
with open(path) as f:
    for l in f:
        info = list(map(int, l.strip().split(" ")))
        results[info[0]] = info[1]


# Sort the dict by keys
lists = sorted(results.items())
x, y = zip(*lists)

fig, ax = plt.subplots()
ax.grid(True)
plt.title("DoS Attack")
plt.xlabel("Number of attackers")
plt.ylabel("Number of reached nodes")

# Plot results
plt.plot(x, y)#, label="Number of reached nodes", linewidth=2)
ax.legend(loc=3)

# Change scale of Xs and Ys
plt.xticks([i for i in range(0, 10001, 1000)])
plt.yticks([i for i in range(0, 10001, 1000)])

# Find the number of attacker for 0%
x = int(ax.lines[0].get_xdata()[np.where(ax.lines[0].get_ydata() == 0)[0][0]])
#plt.scatter(x, 0, c="red")
s = "#Attackers >= {}, total DoS".format(str(x))
#ax.annotate(s, xy=(x, 0), xytext=(x+50, 50))

plt.show()

