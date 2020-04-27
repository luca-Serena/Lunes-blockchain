#!/usr/bin/env bash

# To improve performance move from `grep` to `ripgrep`
mkdir -p ./parsedSelfish/
# For each output file of DoS attack in outputs/ folder
parse_dataSelfish() {
  i=${1}
  filename=$(basename -- "$i")
  filename="${filename%.*}"
  # Get the hasharate
  nodesatkh=$(echo "${i}" | grep -oP 'test-Selfish-\K([0-9]+)')
  if [[ ${nodesatkh} -gt 51 ]]; then
    exit -1
  fi

  ids=()
  while read -r line; do
    l=(${line})
    from=$((l[0] - l[2] + 1))
    for b in $(seq "${from}" "${l[0]}"); do
      ids+=("${b}")
    done
  done < <(grep -Po 'selfish successful \K(-*[0-9]+ -*[0-9]+ [0-9]+)$' "${i}")
  blocks=($(printf "%s\n" "${ids[@]}" | sort -u | tr '\n' ' '))
  for id in "${blocks[@]}"; do
    echo "${id} $(grep -P "BR: [0-9]+,[0-9]+,337,[0-9]+,${id}$" "${i}" | cut -d " " -f2 | cut -d "," -f2 | sort -u | wc -l)" >>"./parsedSelfish/parsedSelfish-${nodesatkh}.txt"
  done
  echo "${i}"
}
export -f parse_dataSelfish

texts() {
  i=${1}
  echo "${i}"
  filename=$(basename -- "$i")
  filename="${filename%.*}"
  blocks_mined=($(grep -Po 'BSS: [0-9]+,337,\K([0-9]+)' "${i}"))
  blocks_mined=($(printf "%s\n" "${blocks_mined[@]}" | sort -nu | tr '\n' ' '))
  for b in "${blocks_mined[@]}"; do
    echo -n "${b} "
    grep -Po "BR: [0-9]+,[0-9]+,337,[0-9]+,${b}$" "${i}" | cut -d " " -f2 | cut -d "," -f2 | sort -u | wc -l
  done

}

# Execute the function in parallel
parallel -j8 parse_dataSelfish ::: ../outputs/test-Selfish-*.txt
