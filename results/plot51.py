#!/usr/bin/env python3
# Script used to plot results from the the test of the 51% attack
# Data is parsed from aggregated results created with ./get_data51.sh
import os
import numpy as np
import re
import matplotlib.pyplot as plt

threshold = 500 // 2  # 50% of nodes received the attacker's block
hashrate = re.compile(r"test-51-(\d+)-parsed.txt")
maxid = re.compile(r"MAXID:\s(\d{1,4})")
rcvn = re.compile(r"\d+\s(\d+)")
#rcvn = re.compile(r"recv: (\d+)")
# CHANGE THE FOLDER TO READ THE CORRECT FILES
parsedfolder = os.path.abspath("parsed51/")
results = {}

for filename in os.listdir(parsedfolder):
    path = parsedfolder + "/" + filename
    if "parsed.txt" in filename:
        print(filename)
        h = int(re.findall(hashrate, filename)[0])
        count = 0
        for i, line in enumerate(open(path)):
            for match in re.finditer(maxid, line):
                thismax = int(match.group(1))
            for match in re.finditer(rcvn, line):
                r = int(match.group(1))
                if r > threshold:
                    count += 1
        # k: hashrate, v: %success
        results[h] = (count / thismax) * 100

assert (len(results) == 100)

# Sort the dict by keys
lists = sorted(results.items())
# unzip the dict
x, y = zip(*lists)
fig, ax = plt.subplots()
ax.grid(True)
plt.title("51% Attack")
plt.xlabel("Attacker's hashrate")
plt.ylabel("% of success")

# Plots the results
plt.plot(x, y, label="% of success")
ax.legend(loc=2)

# Change scale of Xs and Ys
plt.xticks([i for i in range(0, 101, 10)])
plt.yticks([i for i in range(0, 101, 10)])

# Find the % of success with 51% hashrate
y = ax.lines[0].get_ydata()[np.where(ax.lines[0].get_xdata() == 51)[0][0]]
plt.scatter(51, y, c="red")
s = "Hashrate 51%, success: {}%".format(round(y, 2))
ax.annotate(s, xy=(51 + 1, y - 1))

# Find the % of success with 100% hashrate
y = ax.lines[0].get_ydata()[np.where(ax.lines[0].get_xdata() == 100)[0][0]]
plt.scatter(100, y, c="red")
y = 100 if y > 100 else y
s = "Hashrate 100%, success: {}%".format(round(y, 2))
ax.annotate(s, xy=(100 - 40, y + 1))


# Find the hashrate for a ~50% of success
for i, v in enumerate(ax.lines[0].get_ydata()):
    if int(v) == 50:
        x = ax.lines[0].get_xdata()[i]
        s = "Hashrate {}%, success: 50%".format(x)
        #plt.scatter(x - 0.4, v - 0.6, c="red")
        #ax.annotate(s, xy=(x - 22, v))
        break
plt.show()
