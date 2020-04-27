#!/usr/bin/env bash

rm ./parsedDOS/parsedDOS.txt
mkdir -p ./parsedDOS/
# For each output file of DoS attack in outputs/ folder
parse_dataDOS() {
  i=${1}
  echo "${i}"
  filename=$(basename -- "$i")
  filename="${filename%.*}"
  echo "Numero transazioni create dalla vittima:"
  grep -Pc "TS: [0-9]{1,4},1337," "${i}"
  echo "Numero blocchi creati dalla vittima:"
  grep -Pc "BS: [0-9]{1,4},1337," "${i}"
  echo "Numero transazioni ricevute dai nodi e generate dalla vittima:"
  grep -Pc "TR: [0-9]+,[0-9]+,1337," "${i}"
  echo "Numero blocchi ricevuti dai nodi e creati dalla vittima:"
  grep -Pc "BR: [0-9]+,[0-9]+,1337," "${i}"
  #../get_results.py "${i}" >"./parsed51/${filename}-parsed.txt"
  nodesatk=$(echo "${i}" | grep -oP 'test-DOS-\K([0-9]+)')
  echo "${nodesatk} $(grep -P " [0-9]{1,4},[0-9]{1,4},1337," "${i}" | cut -d " " -f2 | cut -d "," -f2 | sort -u | wc -l)" >>./parsedDOS/parsedDOS.txt
  sort -un -o ./parsedDOS/parsedDOS.txt ./parsedDOS/parsedDOS.txt
}
export -f parse_dataDOS

print_nodes_for_each_tx() {
  txids=($(grep -P "TS: [0-9]{1,4},1337," "${1}" | cut -d " " -f2 | cut -d "," -f3))
  c=0
  for tid in "${txids[@]}"; do
    echo "${c}: $(grep -P "TR: [0-9]+,[0-9]+,1337,[0-9]+,${tid}," "${1}" | cut -d " " -f2 | cut -d "," -f2 | sort -u | wc -l)",
    c=$((c + 1))
  done
}
export -f print_nodes_for_each_tx

print_nodes_for_each_block() {
  blocksids=($(grep -Po "BS: [0-9]+,1337,\\K([0-9]+)" "${1}" | cut -d " " -f2 | cut -d "," -f3))
  c=0
  for bid in "${blocksids[@]}"; do
    echo "${c}: $(grep -P "BR: [0-9]+,[0-9]+,1337,[0-9]+,${bid}," "${1}" | cut -d " " -f2 | cut -d "," -f2 | sort -u | wc -l),"
    c=$((c + 1))
  done
}
export -f print_nodes_for_each_block

# Execute the function in parallel
parallel -j9 parse_dataDOS ::: ../outputs/test-DOS-*.txt
