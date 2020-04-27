# LUNES-Bitcoin

### Large Unstructured NEtwork Simulator (LUNES) for Bitcoin

This _LUNES_ implementation is used to simulate the Bitcoin's protocol with options to run some known attacks.

## Features

- Mining
- Transactions
- Block propagation
- Blockchain history
- Verbose output

## Attacks

- 51%
- DoS
- Selfish Mining

## Known Problems

To reduce RAM usage all messages are pointers. This setup cannot be used with multiple _LPs_. The program must be run without parallelism.

`run-bitcoin` script is configured to execute the program and the simulator manager with 1 _LP_.

If your device has more than 16GB (at least, 36GB should be enough) you can change all function calls to pass by value all messages.

## Compilation

Run `make` inside this folder to compile the binary: `bitcoin`.

## Usage

First of all to create a corpus for the simulator run `./make-corpus`.

To start the run use `run-bitcoin` Bash script: this will take care of sourcing and setting all global variables.

This script is used to start the SImulator MAnager (_SIMA_) and the binary `bitcoin` with all necessary arguments.

```
USAGE: ./run-bitcoin --nodes|-n #SMH [--test|-t TESTNAME] [--debug|-d DEBUGCMD]
	#SMH	   total number of nodes to simulate
	[TESTNAME] name of the test to execute: 51, selfish or dos (optional)
	[DEBUGCMD] used for injecting *trace commands (optional)
```

For example to run a simple simulation execute `./run-bitcoin -n 10000`. This will prints all logs in `stdout`. Logs are enabled or disable using `#define` statements in [`sim-parameters.h`](./sim-parameters.h).

```c
// stale blocks debug: prints all msgs with rejected blocks
#define STALEBLOCKDEBUG
//
// rejected transactions debug: prints all msgs
#define STALETXDEBUG
//
// transactions debug: prints all received and sent txs msgs
#define TXDEBUG
//
// request for specific blocks from peers: prints all msg to ask a peer for a specific block
#define ASKBLOCKDEBUG
```

Logs for mined and received blocks are enabled by default.

**Warning**: enabling logs will reduces performances and increments RAM usage.

To execute some attack scenarios user the `--test` flag with `51`, `selfish` or `dos`. This flag initialize the simulator to run ALL tests with this configurations. Each run's output is saved in a `txt` file inside the folder `./outputs`.

For example running `./run-run-blockid -n 10000 -t 51` will perform `100` tests with a single attacker with increasing hashrate (from 1 to 100). Outputs are collected in the `./outputs` folder.

### Outputs

Simulation output is composed of a `tag` and a `data` section (like a CSV file). Tags define what kind of output the program is printing:

- `BS`: a `B`lock is mined and is `S`ent to all the neighbors (`BlockMsg`);
- `BR`: a `B`lock is `R`eceived (`BlockMsg`);
- `TS`: a `T`ransaction is created and `S`ent to all the neighbors (`TransMsg`);
- `TR`: a `T`ransaction is `R`eceived (`TransMsg`);
- `BRS`: a `B`lock is received but it's `S`tale (`BlockMsg`);
- `TRS`: a `T`ransaction is received but it's `S`tale (`TransMsg`);
- `ABS`: a `A`sk`B`lock message is `S`ent (`AskMsg`);
- `ABR`: a `A`sk`B`lock message is `R`eceived (`AskMsg`)

The format of each data section is:

- `BS`: `clock - nodeid - blockminedid - hashrate`
- `BR`: `clock - nodeid - originalcreartor - timestamp - blockid`
- `TS`: `clock - nodeid - txid - from - to - blockid`
- `TR`: `clock - nodeid - originalcreartor - timestamp - txid - from - to - blockid`
- `BRS`: `clock - nodeid - originalcreartor - timestamp - blockid - latestblock`
- `TRS`: `clock - nodeid - originalcreartor - timestamp - txid - from - to`
- `ABS`: `clock - nodeid - blockid`
- `ABR`: `clock - nodeid - blockid - tonode`

Executions outputs can be parsed using the Python script [`get_results.py`](./get_results.py). The script can plots all mined block within a time for all nodes or for a specified node ID, prints the blockchain stored for a node, the best miner and find if a block has received a transaction. There's no main routine and no argument parsing (for the moment). Output file size is at least `200MB` without enabling extra logging.

`python get_results.py test.txt`

Each line of the output is parsed and mapped into a dictionary and inserted into an array:

```python
# BS  clock - nodeid - blockminedid - hashrate
def parse_mined_block(arr):
    data = list(map(str.strip, arr.split(",")))
    if len(data) == 4:
        return dict(zip(["clock", "nodeid", "blockid", "hashrate"], data))
    else:
        print("Wrong set", inspect.stack()[0][3])
```

This script requires Python3 and [`matplotlib`](https://matplotlib.org/) installed.

For more informations about data parsing and aggregation see `./results/README.md`.

## Documentation ([Doxygen](https://github.com/doxygen/doxygen)

Run `doxygen` in this folder to generate a HTML and a LaTeX report.

The command will generate a `/doc` folder (ignored by the repository indexing) with two subdirectories:

- `html`: for the HTML report (open the `index.html`)
- `latex`: run `make` inside this folder to generate the PDF report named `refman.pdf`

## 51%

This execution mode will run 100 times the main simulation using an increasing value of the hashrate of an attacker (using 10000 node the `attackerid` is 1337).

Output of this tests can be found in `./outputs` directory with the name: `test-51-H.txt` with **H** the value of the attacker's hashrate.

#### Tests

- 51-v1: 70% of miners (7000) in the network with low hashrate (no pools); the final plot is not what theoretical should be expected
- 51-v2: 0.2% of miners (20) in the network with two pools with hashrate 17.7 and 13.8;
- 51-v3: 0.15% of miners in the network with 3 pools and attacker is joining the network increasing the total hashrate (the sum of all hashrates percentage is > 100%);
- dos: clock of 200 with 70% of miners; increasing number of node that perform the filtering of all messages of node 1337
- dos-v2: same as `dos` but with `broadcast` dissemination and `TTL=20` (same results)
- selfish-mining: 0.15% of miners with 3 bit pools

## Contacts

Gabriele D'Angelo: <g.dangelo@unibo.it>
Edoardo Rosa: <edoardo.rosa@studio.unibo.it>
