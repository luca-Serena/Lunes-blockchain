# LUNES-Blockchain

### Large Unstructured NEtwork Simulator (LUNES) for Blockchain

This _LUNES_ implementation is used to simulate the Bitcoin's protocol with options to run some known attacks.

## Features

- Mining
- Transactions
- Block and transaction propagation
- Blockchain concurrent behaviour
- Forking: it is possible to use the old version with no chain forks allowed or the new version, in which the blocks actually have an ID, a reference to the previous block, a certain position in a chain and the program keeps track of a given number (Default 10) of chain forks, forgetting gradually the shortest chains

## Attacks

- 51%
- DoS
- Selfish Mining

## Known Problems

To reduce RAM usage all messages are pointers. This setup cannot be used with multiple _LPs_. The program must be run without parallelism.

`run-blockchain` script is configured to execute the program and the simulator manager with 1 _LP_.

If your device has more than 16GB (at least, 36GB should be enough) you can change all function calls to pass by value all messages.

## Compilation

Run `make` inside this folder to compile the binary: `blockchain`.

## Usage

First of all to create a corpus for the simulator run `./make-corpus <#NODES> <#EDGES> <#MAX_DIAMETER>.` 
For example  ./make-corpus 1000 4 8 will create a graph with 1000 nodes, 4000 edges and a maximum diameter minor or equal than 8

To start the run use `run-blockchain` Bash script: this will take care of sourcing and setting all global variables.

This script is used to start the SImulator MAnager (_SIMA_) and the binary `blockchain` with all necessary arguments.

```
USAGE: ./run-blockchain --nodes|-n #SMH [--test|-t TESTNAME] [--debug|-d DEBUGCMD]
	#SMH	   total number of nodes to simulate
	[TESTNAME] name of the test to execute: 51, selfish or dos (optional)
	[DEBUGCMD] used for injecting *trace commands (optional)
```

For example to run a simple simulation execute `./run-blockchain -n 1000`. This will prints all logs in `stdout`. Logs are enabled or disabled using the `#define` statements in [`sim-parameters.h`](./sim-parameters.h).

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

// activation of the new configuration with forking allowed
#define FORKING
//
// defining DOS it's possible to simulate the Denial of Service attack
#define DOS
```

Logs for mined and received blocks are enabled by default.

**Warning**: enabling logs will reduces performances and increments RAM usage.

To execute some attack scenarios use the `--test` flag with `51`, `selfish` or `dos`. This flag initializes the simulator to run ALL tests with this configurations. Each run's output is saved in a `txt` file inside the folder `./outputs`.

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


### Evaluation 

-- forking mode --

To evaluate the outcome of 51% attack run the script evaluate51.sh
The output will be a file named 51-res.txt whose records are in the format:

attacker hashrate - number of blocks in the main chain mined by the attacker -  length of the longest chain


Similarly to evaluate the outcome of the selfish mining attack run the script evaluateSelfish.sh
The output will be a file named selfish-res.txt whose records are in the format:

attacker hashrate - number of succeded attacks

To evaluate the outcome of dos attack run the script evaluateDoS.sh


## Documentation ([Doxygen](https://github.com/doxygen/doxygen))

Run `doxygen` in this folder to generate a HTML and a LaTeX report.

The command will generate a `/doc` folder (ignored by the repository indexing) with two subdirectories:

- `html`: for the HTML report (open the `index.html`)
- `latex`: run `make` inside this folder to generate the PDF report named `refman.pdf`

## 51%

This execution mode will run 100 times the main simulation using an increasing value of the hashrate of an attacker (by default the `attackerid` is 337).
 
Output of this tests can be found in `./outputs` directory with the name: `test-51-H.txt` with **H** the value of the attacker's hashrate.

##Selfish mining
Also here the execution will run 100 times the main simulation using an increasing value of the hashrate of an attacker

Output of this tests can be found in `./outputs` directory with the name: `test-selfish-H.txt` with **H** the value of the attacker's hashrate.

#DOS
The execution is divided into epochs, each one last the same number of steps as the time to live for messages. In every epoch a victim node is chosen and it's evaluated the percentage of nodes among the honest ones that manage to receive a message. 

Output of this tests can be found in `./outputs` directory with the name: `test-dos-H.txt` with **H** the percentage of malicious nodes in the network.

#### Tests

51% attack has been tested both with 9 pools (whose overall hashrate is 82,7%) and without any pool. The pools are dealt as nodes with a very high hashrate, that represent what in the real life are groups of miners who put together their computing power in order to have a better chance to mine a node. The results between the two configuration slightly differ, in the no pools configuration the number of succeded attacks is higher. Then the attack can be tested with different network topologies and with a different number of nodes and edges.

Selfish mining has also been tested both with 9 pools and with no pools. The results are simular but in the no pools configuration the number of succeded attacks is slightly higher.

DoS attack has been tested with the proposed gossip algorithms. It turned out that Dandelion is much more prone to the attack with respect to the other protocols. However using the fail-safe machanism of Dandelion++ it's possible to reach the broadcast coverage levels.

## Contacts

Gabriele D'Angelo: <g.dangelo@unibo.it>

Edoardo Rosa: <edoardo.rosa@studio.unibo.it>

Luca Serena <luca.serena2@unibo.it>
