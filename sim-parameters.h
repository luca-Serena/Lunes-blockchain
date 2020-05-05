/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	Model level "hard coded" simulation parameters
 *              -	Uncomment DEBUG defines for very verbose output
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */


// NOTE:
//	some of the following values can be superseded by environmental variables,
//	see the user_environment_handler() in the file: user_event_handler.c

/****************************** DEBUG *****************************************/
// Uncomment to activate debug
//
//	general debug
//#define DEBUG
//
//	traces the actions of free riding nodes
//#define FREERIDINGDEBUG
//
//	if not defined the tracing of dissemination protocol messages is disabled
//#define TRACE_DISSEMINATION
//
// stale blocks debug
//#define STALEBLOCKDEBUG
//
// rejected transactions debug
//#define STALETXDEBUG
//
// transactions debug
//#define TXDEBUG
//
// request for specific blocks from peers
#define ASKBLOCKDEBUG

#define FORKING


/**************************** MODEL ****************************************/
//just every 1/INTERMEDIATE_STEPS you try to mine a block
#define INTERMEDIATE_STEPS  8

// Simulation length (final clock value), default value
#define END_CLOCK         5000 

// This timestep is chosen to build the aggregation structure
#define BUILDING_STEP     3

// At this timestep the aggregation is completed and the nodes' start pinging each other
#define EXECUTION_STEP    5

// Number of timestep required by ping messages to receive the destination node
//	WARNING: due to synchronization constraints The FLIGHT_TIME has to be bigger
//		than the timestep size
#define FLIGHT_TIME     1.0


/************************ SIMULATOR  LIMITS ********************************/

// Max number of records that can be inserted in a single ping message
#define MAX_PING_DYNAMIC_RECORDS         0

// Max number of records that can be inserted in a single migration message
#define MAX_MIGRATION_DYNAMIC_RECORDS    1000

// Buffer size for incoming messages
//	obviously the buffer needs to be so large to contain all kind of messages
//	(e.g. ping and migration messages)
#define BUFFER_SIZE    1024 * 1024

/***************** DEGREE DEPENDENT GOSSIP *********************************/
#define DEGREE_DEPENDENT_GOSSIP_SUPPORT
