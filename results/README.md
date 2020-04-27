# USAGE

### Requirements

- `parallel` (<https://www.gnu.org/software/parallel/>)
- `matplotlib` (for Python3)
- `numpy` (for Python3)

After running some tests using `run-bitcoin` all results should be in `../outputs/`. This files can be considered as raw data that should be parsed and aggregated.

To aggregate some data you can use the Python script `../get_results.py`; this script parse a single file to prints some informations.
The script is not bulletproof for each tests but some useful functions are implemented.

## 51%

To collect results for this test just run `./get_data51_with_plot.sh`. This script will parse each file in `../outputs/test-51-*.txt` using `get_results.py` to create all plots in `plots/` and some aggregate informations in `parsed-51/` folder. The TXT files inside `parsed-51` can be elaborate using `plot51.py`.

_parsed-51 file format_:

- `MAXID`: the total number of mined blocks (that should be equal to the higher mined block id)
- `node`: the ID of the node attacker (should be 1337)
- `block`: ID of attacker's mined blocks
- `recv`: number of nodes that had received the mined block

The script is very low and RAM eater! For a way faster results use `./get_data51.sh`; this script will not generate all plots but will create files in `./parsed51` used to plot the final graph with `plot51.py`.

## DoS

Once the simulator has completed the run in `./outputs` there will be all files named `test-DOS-????.txt`. Those files can be parsed to generate a list of information used to create a plot.

To aggregate outputs file use `./get_data51.zsh`; this script will not generate all plots but will create files in `./parsedDOS` used to plot the final graph with `plotDOS.py`.

You can still use `../get_results.py` to generate all graphs.

## Selfish Mining

This test adds a new log tag `BSS` with the same format of `BS` and a print if the attack have a possibility to be successful. Using `./get_dataSelfish.sh` is possible to aggregate all data and then using `./plotSelfish.py` it's possible to plot the final graph.
