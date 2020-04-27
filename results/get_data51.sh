#!/usr/bin/env bash

# Faster version of ./get_data51_with_plot.sh without using python
# and without plotting results
# For each output file of 51% attack in outputs/ folder
mkdir -p ./parsed51/
parse_data51() {
  i=${1}
  echo "${i}"
  filename=$(basename -- "$i")
  filename="${filename%.*}"
  blocks_mined=($(grep -Po 'BS: [0-9]+,337,\K([0-9]+)' "${i}")) #337 instead of 1337
  maxid=$(grep -Po "BS: [0-9]+,[0-9]+,\\K([0-9]+)" "${i}" | sort -rn | head -n 1)

  # For each block mined by the attacker with 1337
  # find the number of block that received the block
  # received == inserted into the blockchain
  echo "MAXID: ${maxid}" >"./parsed51/${filename}-parsed.txt"
  # MAXID is need to calculate the % of success
  for b in "${blocks_mined[@]}"; do
    echo -n "${b} " >>"./parsed51/${filename}-parsed.txt"
    grep -Po "BR: [0-9]+,[0-9]+,337,[0-9]+,${b}$" "${i}" | cut -d " " -f2 | cut -d "," -f2 | sort -u | wc -l >>"./parsed51/${filename}-parsed.txt"  #337 instead of 1337
  done
}

export -f parse_data51
parallel -j8 parse_data51 ::: ../outputs/test-51-*.txt
