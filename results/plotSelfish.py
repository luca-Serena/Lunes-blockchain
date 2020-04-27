#!/usr/bin/env python3
# Script used to plot results from the the test of a selfish mining
# Data is parsed from aggregated results created with ./get_dataSelfish.sh
import os
#import matplotlib
#matplotlib.use('Agg')
import re
import matplotlib.pyplot as plt

threshold = 10000 // 2  # 50% of nodes received the attacker's block
hashrate = re.compile(r"parsedSelfish-(\d+).txt")
# CHANGE THE FOLDER TO READ THE CORRECT FILES
parsedfolder = os.path.abspath("parsedSelfish/")
results_single = {}
results_pairs = {}


def get_data(f):
    with open(f) as data:
        lines = [l.strip().split(" ") for l in data if "MAXID" not in l]
        lines = [list(map(int, l)) for l in lines]
        index = 0
        count = 0
        while index < len(lines) - 1:
            current = lines[index]
            follow = lines[index + 1]
            index += 1
            # if two consective blocks have reached >50% nodes
            if current[0] + 1 == follow[
                    0] and current[1] > threshold and follow[1] > threshold:
                index += 1
                count += 1
        return sum(i[1] > threshold for i in lines), count


for filename in os.listdir(parsedfolder):
    if "parsedSelfish" in filename:
        path = parsedfolder + "/" + filename
        h = int(re.findall(hashrate, filename)[0])
        if h <= 50:
            data = get_data(path)
            results_single[h] = data[0]
            results_pairs[h] = data[1]

# Create a baseline
parsedfolder = os.path.abspath("parsed51/")
hashrate = re.compile(r"test-51-(\d+)-parsed.txt")
results51_single = {}
results51_pairs = {}
for filename in os.listdir(parsedfolder):
    if "parsed.txt" in filename:
        path = parsedfolder + "/" + filename
        h = int(re.findall(hashrate, filename)[0])
        if h <= 50:
            data = get_data(path)
            results51_single[h] = data[0]
            results51_pairs[h] = data[1]

for i in range(1, 51):
    if i not in results_pairs:
        results_pairs[i] = 0
    if i not in results_single:
        results_single[i] = 0
    if i not in results51_pairs:
        results51_pairs[i] = 0
    if i not in results51_single:
        results51_single[i] = 0

fig, ax = plt.subplots()
# Change scale of Xs and Ys
plt.yticks([i for i in range(0, 901, 100)])
ax.grid(True)
plt.title("Selfish Mining")
plt.xlabel("Attacker's hashrate")

# Sort the dict by keys
lists = sorted(results_single.items())
# unzip the dict
x, y = zip(*lists)
# Plots the results
plt.plot(x, y, label="Accepted blocks")
# Sort the dict by keys
lists = sorted(results_pairs.items())
# unzip the dict
x, y = zip(*lists)
plt.plot(x, y, label="Accepted pairs of consecutive blocks")

# Plot baseline
# Sort the dict by keys
lists = sorted(results51_single.items())
# unzip the dict
x, y = zip(*lists)
# Plots the results
plt.plot(x, y, label="Accepted blocks (normal)")
# Sort the dict by keys
lists = sorted(results51_pairs.items())
# unzip the dict
x, y = zip(*lists)
plt.plot(x, y, label="Accepted pairs of consecutive blocks (normal)")

ax.legend(loc=2)
plt.show()
