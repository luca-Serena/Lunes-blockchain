#!/usr/bin/env bash
###############################################################################################
#	Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
#	Large Unstructured NEtwork Simulator (LUNES)
################################################################################################

#########################################################
#	CONFIGURATION FILES USED BY SCRIPTS
#########################################################
#
#	Common prefix for the output directories
PREFIX_DIRECTORY=/srv/lunes
#
#	Directory in which will be placed the simulation traces
TRACE_DIRECTORY=$PREFIX_DIRECTORY/traces
#
#	Directory in which will be executed the statistics
WORKING_DIRECTORY=$PREFIX_DIRECTORY/working
#
#	Directory in which will be placed the results
RESULTS_DIRECTORY=$PREFIX_DIRECTORY/results
#
#	Directory in which is a corpus of graphs
#	that can be used for the different runs
CORPUS_DIRECTORY=$PREFIX_DIRECTORY/corpus
#
mkdir -p $TRACE_DIRECTORY $WORKING_DIRECTORY $RESULTS_DIRECTORY $CORPUS_DIRECTORY
#
#########################################################
#	EXECUTION VALUES
#########################################################
#
#	Number of working threads to be run for log processing
SOCKETS=$(lscpu | grep 'Socket(s)' | awk '{print $NF}')
COREPERSOCKET=$(lscpu | grep 'Core(s) per socket' | awk '{print $NF}')
CPUNUM=$(($SOCKETS * $COREPERSOCKET))
#
#	Reduce the I/O priority of stats workers
IONICE='ionice -c 3'
#
#	Number of LPs to be used:
#		1 = monolithic (sequential) simulation
#		> 1 parallel simulation
LPS=1
#
#	Total number of runs per each configuration
#	(for statistical purposes)
NUMBERRUNS=1
#
#       Numerical value of the first run
#       (it can useful to produce results that will be combined together at a later stage)
STARTINRUNVALUE=0
#
#	Turn on the statistics calculation
STATISTICS=1
#
#	Turn on the simulation execution
EXECUTION=1
#
#	Delete all trace files after statistics calculation
NOTRACE=1
#
#	Networks to be evaluated
#		the smallest network has this size
START_NUMBER_NODES=500
#
#		the largest one has this size
END_NUMBER_NODES=500
#
#	Step-by-step evaluation form the smallest to the biggest
STEP_NODES=100
#
#	Number of time-steps in each simulation run
#	(after the building phase of the network is completed)
export END_CLOCK=1000

#########################################################
# File names definition, used for statistics purposes
#########################################################
#
#	output files
#
DELAYS="STAT_msg_delays.txt"
MEAN="STAT_mean_delay.txt"
MSGIDS="STAT_msg_ids.txt"
NODES="STAT_nodes_ids.txt"
OUTPUT="STAT_coverage.txt"
DISTRIBUTION="STAT_missing_distribution.txt"
#
#	temporary files
#
RUNSCOVERAGEMEANTMP="STAT_tmp_runs_coverage_mean.txt"
RUNSCOVERAGETMP="STAT_tmp_runs_coverage.txt"
RUNSDELAYMEANTMP="STAT_tmp_runs_delay_mean.txt"
RUNSDELAYTMP="STAT_tmp_runs_delay.txt"
TMP="STAT_tmp_coverage.txt"
RUNSMESSAGETMP="STAT_tmp_runs_messages.txt"
RUNSMESSAGEMEANTMP="STAT_tmp_runs_messages_mean.txt"
