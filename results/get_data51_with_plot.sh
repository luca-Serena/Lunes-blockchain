#!/usr/bin/env bash
# NB: very long execution time and RAM usage (16GB is enough though)

# This script will execute the Python script to generate all graphs for the 51% attack and collects all data to build the final graph (plot percentage of success by hashrate)
mkdir -p ./parsed51/
for i in ../outputs/test-51-*.txt; do
  echo "${i}"
  filename=$(basename -- "$i")
  filename="${filename%.*}"
  echo "${filename}"
  ../get_results.py "${i}" >"./parsed51/${filename}-parsed.txt"
done
