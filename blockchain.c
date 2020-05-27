#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <ini.h>
#include <ts.h>
#include <rnd.h>
#include <gaia.h>
#include "utils.h"
#include "user_event_handlers.h"

/*-------- G L O B A L     V A R I A B L E S --------------------------------*/

int NSIMULATE,                // Number of Interacting Agents (Simulated Entities) per LP
    NLP,                      // Number of Logical Processes
    LPID,                     // Identification number of the local Logical Process
    local_pid;                // Process Identifier (PID)

double *rates;
// < 0: disabled
int atk_hashrate     = -1;
int number_dos_nodes = -1;
// "Data structure" to save, attacaker, blockchain and attack status
// selfish[0] = latest mined block in attacker's private blockchain
// selfish[1] = latest received block from the network for the attacker
// selfish[2] = status of the attack (-1 = disabled, 0 active and even with the blockchain, 1
//              attacker is 1 block ahead, 3 attacker sent the mined block
int *selfish;
int *attackers;

// SImulation MAnager (SIMA) information and localhost identifier
static char LP_HOST[64];      // Local hostname
static char SIMA_HOST[64];    // SIMA execution host (fully qualified domain)
static int  SIMA_PORT;        // SIMA execution port number

// Time management variables
double step,                  // Size of each timestep (expressed in time-units)
       simclock        = 0.0; // Simulated time
static int end_reached = 0;   // Control variable, false if the run is not finished

// A single LP is responsible to show the runtime statistics
//	by default the first started LP is responsible for this task
static int LP_STAT = 0;

// Seed used for the random generator
TSeed Seed, *S = &Seed;
/*---------------------------------------------------------------------------*/

// File descriptors:
//	lcr_fp: (output) -> local communication ratio evaluation
//	finished: (output) -> it is created when the run is finished (used for scripts management)
//
FILE *lcr_fp, *finished_fp;

// Output directory (for the trace files)
char *TESTNAME;

// Simulation control (from environment variables, used by batch scripts)
unsigned int   env_migration;                 // Migration state
float          env_migration_factor;          // Migration factor
unsigned int   env_load;                      // Load balancing
float          env_end_clock = END_CLOCK;     //* INTERMEDIATE_STEPS;     // End clock (simulated time)
unsigned short env_dissemination_mode;        // Dissemination mode
float          env_broadcast_prob_threshold;  // Dissemination: conditional broadcast, probability threshold
unsigned int   env_cache_size;                // Cache size of each node
float          env_fixed_prob_threshold;      // Dissemination: fixed probability, probability threshold
float          env_dandelion_fluff_steps;     // Dissemination: number of stem and fluff phase
float          env_freerider_prob;            // Probability that a given node is a free-rider
int            number_of_heads;
#ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
unsigned int   env_probability_function;      // Probability function for Degree Dependent Gossip
double         env_function_coefficient;      // Coefficient of the probability function
#endif
float  env_global_hashrate;                   /* Total Hashrate of Bitcoin Network in H/min */
double env_difficulty;                        /* Actual Bitcoin network difficulty */
int    env_miners_count;                      /* Number of miners for this current run */
#ifdef DOS
int victim = -1;                              /* ID of the victim node. It's incremented each epoch*/
#endif
/* ************************************************************************ */
/*                      Hash Tables		                                    */
/* ************************************************************************ */

hash_t hash_table, *table = &hash_table; /* Global hash table, contains ALL the simulated entities */
hash_t sim_table, *stable = &sim_table;  /* Local hash table, contains only the locally managed entities */
/*---------------------------------------------------------------------------*/

/* ************************************************************************ */
/*             Migrating Objects List			                            */
/* ************************************************************************ */

// List containing the objects (SE) that have to migrate at the end of the
//	current timestep
static se_list migr_list,
               *mlist = &migr_list;
/*---------------------------------------------------------------------------*/


/* *************************************************************************
 *                    M O D E L    D E F I N I T I O N
 *      NOTE: in the following there is only a part of the model definition,
 *      the most part of it is implemented in the user level,
 *      see: user_event_handlers.c and lunes.c
 **************************************************************************** */

/*! \brief Computation and Interactions generation: called at each timestep
 *         it will provide the model behavior.
 */
static void Generate_Computation_and_Interactions(int total_SE) {
    // Call the appropriate user event handler
    user_control_handler();
}

/*! \breif SEs initial generation: called once when global variables have been
 *         initialized.
 */
static void Generate(int count) {
    int i;

    // The local Simulated Entities are registered using the appropriate GAIA API
    for (i = 0; i < count; i++) {
        // In this case every entity can be migrated
        GAIA_Register(MIGRABLE);

        // NOTE: the internal state of entities is initialized in the
        //		register_event_handler()
        //		see in the following of this source file
    }
}

/*! \brief Performs the migration of the flagged Simulated Entities
 */
static int UNUSED ScanMigrating() {
    // Current entity
    struct hash_node_t *se = NULL;

    // Migration message
    MigrMsg m;

    // Number of entities migrated in this step, in this LP
    int migrated_in_this_step = 0;

    // Cursor used for the local state of entities
    int state_position;

    // Total size of the message that will be sent
    unsigned int message_size;

    // Iterator to scan the whole state hashtable of entities
    GHashTableIter iter;
    gpointer       key, value;


    // The SEs to migrate have been already identified by GAIA
    //	and placed in the migration list (mlist) when the
    //	related NOTIF_MIGR was received
    while ((se = list_del(mlist))) {
        // Statistics
        migrated_in_this_step++;

        // A new "M" (migration) type message is created
        m.migration_static.type = 'M';

        // The state of each IA is composed of a set of elements
        //	let's start from the first one
        state_position = 0;

        // The static part of the agents state has to be inserted in
        //	the migration message
        m.migration_static.s_state = se->data->s_state;

        // Dynamic part of the agents state
        //
        // The hashtable is empty
        if (se->data->state == NULL) {
            #ifdef DEBUG
            fprintf(stdout, "ID: %d is empty\n", se->data->key);
            fflush(stdout);
            #endif
        }

        // Copying the local state of the migrating entity in the payload of the migration message
        //	for each record in the entity state a new record is appended in the dynamic part
        //	of the migration message
        if (se->data->state != NULL) {
            #ifdef DEBUG
            int tmp = 0;
            #endif

            // Hashtable iterator
            g_hash_table_iter_init(&iter, se->data->state);

            while (g_hash_table_iter_next(&iter, &key, &value)) {
                m.migration_dynamic.records[state_position].key      = *(unsigned int *)key;
                m.migration_dynamic.records[state_position].elements = *((value_element *)value);
                #ifdef DEBUG
                tmp++;

                fprintf(stdout, "%12.2f node: [%5d] migration, copied key: %d, (%4d/%4d)\n", simclock, se->data->key, m.migration_dynamic.records[state_position].key, tmp, g_hash_table_size(se->data->state));

                fflush(stdout);
                #endif

                state_position++;
            }
        }

        // It is time to clean up the hash table of the migrated node
        if (se->data->state != NULL) {
            // In the hash table creation it has been provided the cleaning function that is g_free ()
            g_hash_table_destroy(se->data->state);
        }

        // Calculating the real size of the migration message
        message_size = sizeof(struct _migration_static_part);

        if (message_size >= BUFFER_SIZE) {
            // I'm trying to send a message that is larger than the buffer
            fprintf(stdout, "%12.2f node: FATAL ERROR, trying to send a message (migration) that is larger than: %d !\n", simclock, BUFFER_SIZE);
            fflush(stdout);
            exit(-1);
        }

        // The migration is really executed
        GAIA_Migrate(se->data->key, (void *)&m, message_size);

        // Removing the migrated SE from the local list of migrating nodes
        hash_delete(LSE, stable, se->data->key);
    }

    // Returning the number of migrated SE (for statistics)
    return(migrated_in_this_step);
}

/*---------------------------------------------------------------------------*/

/* ************************************************************************ */
/*           E V E N T   H A N D L E R S			                        */
/* ************************************************************************ */

/*! \brief Upon arrival of a model level event, firstly we have to validate it
 *         and only in the following the appropriate handler will be called
 */
struct hash_node_t *validation_model_events(int id, int to, Msg *msg) {
    struct hash_node_t *node;

    // The receiver has to be a locally manager Simulated Entity, let's check!
    if (!(node = hash_lookup(stable, to)))  {
        // The receiver is not managed by this LP, it is really a fatal error
        fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] is NOT in this LP!\n", simclock, to);
        fflush(stdout);
        exit(-1);
    }else {
        return(node);
    }
}

/*! \brief A new SE has been created, we have to insert it into the global
 *      and local hashtables, the correct key to use is the sender's ID
 */
static void register_event_handler(int id, int lp) {
    hash_node_t *node;

    // In every case the new node has to be inserted in the global hash table
    //	containing all the Simulated Entities
    node = hash_insert(GSE, table, NULL, id, lp);
    if (node) {
        node->data->s_state.changed = YES;
        // If the SMH is local then it has to be inserted also in the local
        //	hashtable and some extra management is required
        if (lp == LPID) {
            // Call the appropriate user event handler
            user_register_event_handler(node, id);

            // Inserting it in the table of local SEs
            if (!hash_insert(LSE, stable, node->data, node->data->key, LPID)) {
                // Unable to allocate memory for local SEs
                fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] impossible to add new elements to the hash table of local entities\n", simclock, id);
                fflush(stdout);
                exit(-1);
            }
        }
    }else {
        // The model is unable to add the new SE in the global hash table
        fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] impossible to add new elements to the global hash table\n", simclock, id);
        fflush(stdout);
        exit(-1);
    }

    fprintf(stdout, "NODE %d: HASHRATE %f%%; IS MINER? %d; IS ATTACKER? %d;\n", node->data->key, node->data->hashrate, node->data->miner, node->data->attackerid);
    fflush(stdout);
}

/*! \brief Manages the "migration notification" of local SEs (i.e. allocated in this LP)
 */
static void notify_migration_event_handler(int id, int to) {
    hash_node_t *node;

    #ifdef DEBUG
    fprintf(stdout, "%12.2f agent: [%5d] is going to be migrated to LP [%5d]\n", simclock, id, to);
    #endif

    // The GAIA framework has decided that a local SE has to be migrated,
    //	the migration can NOT be executed immediately because the SE
    //	could be the destination of some "in flight" messages
    if ((node = hash_lookup(table, id)))  {
        /* Now it is updated the list of SEs that are enabled to migrate (flagged) */
        list_add(mlist, node);

        node->data->lp = to;
        node->data->s_state.changed = YES;

        // Call the appropriate user event handler
        user_notify_migration_event_handler();
    }
    // Just before the end of the current timestep, the migration list will be emptied
    //	and the pending migrations will be executed
}

/*! \brief Manages the "migration notification" of external SEs
 *         (that is, NOT allocated in the local LP).
 */
static void notify_ext_migration_event_handler(int id, int to) {
    hash_node_t *node;

    // A migration that does not directly involve the local LP is going to happen in
    //	the simulation. In some special cases the local LP has to take care of
    //	this information
    if ((node = hash_lookup(table, id)))  {
        node->data->lp = to;                // Destination LP of the migration
        node->data->s_state.changed = YES;
        // Call the appropriate user event handler
        user_notify_ext_migration_event_handler();
    }
}

/*\brief Migration-event manager (the real migration handler)
 *       This handler is executed when a migration message is received and
 *       therefore a new SE has to be accomodated in the local LP.
 */
static void  migration_event_handler(int id, Msg *msg) {
    hash_node_t *node;

    #ifdef DEBUG
    fprintf(stdout, "%12.2f agent: [%5d] has been migrated in this LP\n", simclock, id);
    #endif

    if ((node = hash_lookup(table, id))) {
        // Inserting the new SE in the local table
        hash_insert(LSE, stable, node->data, node->data->key, LPID);

        // Call the appropriate user event handler
        user_migration_event_handler(node, id, msg);
    }
}

/*---------------------------------------------------------------------------*/


/* ************************************************************************ */
/*                  U T I L S				                                */
/* ************************************************************************ */

/*! \brief Loading the configuration file of the simulator
 */
static void UNUSED LoadINI(char *ini_name) {
    int  ret;
    char data[64];


    ret = INI_Load(ini_name);
    ASSERT(ret == INI_OK, ("Error loading ini file \"%s\"", ini_name));

    /* SIMA */
    ret = INI_Read("SIMA", "HOST", data);
    if (ret == INI_OK && strlen(data)) {
        strcpy(SIMA_HOST, data);
    }

    ret = INI_Read("SIMA", "PORT", data);
    if (ret == INI_OK && strlen(data)) {
        SIMA_PORT = atoi(data);
    }

    INI_Free();
}

/*! \breif Generate random values for hashrates (in %) for each node
 *
 * @param[in] nodes: Number of nodes
   Nine pools are created with the aim to simulate the biggest pools of miners, largely bigger than other ones
 */
static void generate_hashrates(int nodes) {
    // We need to fix some hashrate to simulate some mining pool
    //F2pool
    double pool0 = 19.5;
    // BTC.com hashrate
    double pool1 = 17.1;
    // AntPool hashrate
    double pool2 = 10.6;
    // Poolin hashrate
    double pool3 = 14.6;
    //Via BTC
    double pool4 = 6.8;
    // BTC.TOP
    double pool5 = 5;
    //slashpool
    double pool6 = 4.1;
    //bitfury
    double pool7= 2.3;
    //bitcoin.com
    double pool8= 0.4;
    double pools = pool0 + pool1 + pool2 + pool3 + pool4 + pool5 + pool6 + pool7 + pool8;

    if (atk_hashrate != 0 && atk_hashrate != -1){     
        pool1 -= pool1 * atk_hashrate / 100;
        pool2 -= pool2 * atk_hashrate / 100;
        pool3 -= pool3 * atk_hashrate / 100;
        pool0 -= pool0 * atk_hashrate / 100;
        pool4 -= pool4 * atk_hashrate / 100;
        pool5 -= pool5 * atk_hashrate / 100;
        pool6 -= pool6 * atk_hashrate / 100;
        pool7 -= pool7 * atk_hashrate / 100;
        pool8 -= pool8 * atk_hashrate / 100;
        pools = pool0 + pool1 + pool2 + pool3 + pool4 + pool5 + pool6 + pool7 + pool8 + atk_hashrate;
    }

    rates = (double *)malloc(nodes * sizeof(double));
    double sum = 0;
    rates[0] = pool0;
    rates[1] = pool1;
    rates[2] = pool2;
    rates[3] = pool3;
    rates[4] = pool4;
    rates[5] = pool5;
    rates[6] = pool6;
    rates[7] = pool7;
    rates[8] = pool8;
    for (int i = 9; i < env_miners_count; ++i) {
        double val = RND_Interval(S, 0.0, 100.0);
        sum     += val;
        rates[i] = val;
    }

    // resize the pools in order to maintain the proportion (and the 100% of overall hashrate) even 
    // with the presence of an attacker with a given hashrate
    double desired = 100.0 - pools;
    for (int i = 9; i < env_miners_count; i++) {
        rates[i] = rates[i] / sum * desired;
    }
}
/*---------------------------------------------------------------------------*/


/* ************************************************************************ */
/*                  M A I N				                                    */
/* ************************************************************************ */

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Not enough arguments:\n");
        printf("\tUSAGE: ./blockchain #NLP #IA #TRACEPATH\n");
    }
    char msg_type,                      // Type of message
         *data,                         // Buffer for incoming messages, dynamic allocation
         *rnd_file = "Rand2.seed";      // File containing seeds for the random numbers generator

    int count,                          // Number of SEs to simulate in the local LP
        start,                          // First identifier (ID) to be used to tag the locally managed SEs
        max_data;                       // Maximum size of incoming messages

    int from,                           // ID of the message sender
        to,                             // ID of the message receiver
        tot = 0;                        // Total number of executed migrations

    int loc,                            // Number of messages with local destination (intra-LP)
        rem,                            // Number of messages with remote destination (extra-LP)
        migr;                           // Number of executed migrations
    //int t;                              // Total number of messages (local + remote)

    double Ts;                          // Current timestep
    Msg *  msg;                         // Generic message

    //int migrated_in_this_step;          // Number of entities migrated in this step, in the local LP

    struct hash_node_t *tmp_node;       // Tmp variable, a node in the hash table
    char *dat_filename, *tmp_filename;  // File descriptors for simulation traces

    // Time measurement
    struct timeval t1, t2;

    // Local PID
    local_pid = getpid();

    // Loading the input parameters from the configuration file
    LoadINI("blockchain.ini");

    // Returns the standard host name for the execution host
    gethostname(LP_HOST, 64);

    // Command-line input parameters
    NLP       = atoi(argv[1]);  // Number of LPs
    NSIMULATE = atoi(argv[2]);  // Number of SEs to simulate
    TESTNAME  = argv[3];        // Output directory for simulation traces
    // Set selfish disabled
    selfish    = (int *)malloc(3 * sizeof(int));
    selfish[0] = -1;
    selfish[1] = -1;
    selfish[2] = -1;
    attackers= (int *)malloc(1 * sizeof(int));         //list of malicious nodes, read from attackers.txt

    if (argc > 4) {
        if (strcmp(argv[4], "--attacker") == 0) {
            atk_hashrate = atoi(argv[5]);
        }
        if (strcmp(argv[4], "--dos") == 0) {
            number_dos_nodes = atoi(argv[5]);
            float coefficient = (float) NSIMULATE / 100;
            number_dos_nodes = coefficient * number_dos_nodes;
            fprintf(stdout, "%d %d\n", number_dos_nodes, NSIMULATE);

            FILE *fp = fopen("attackers.txt", "r");
            if(fp == NULL) {
                perror("Unable to open file!");
                exit(1);
            }
            attackers= (int *)realloc(attackers,(number_dos_nodes - 1) * sizeof(int));        //memorizing the array of attackers
            char *line = NULL;
            size_t len = 5;
            int iter = 0;

            while(getline(&line, &len, fp) != -1 && iter < number_dos_nodes) {
                 int t = atoi (line);
                 attackers[iter] = t;
                 iter++;
            }

           /* if (number_dos_nodes >= victim){   //managed differently in the new version of dos
                number_dos_nodes ++;
            }*/
        }
        if (strcmp(argv[4], "--selfish") == 0) {
            atk_hashrate = atoi(argv[5]);
            selfish[0]   = -1;
            selfish[1]   = -1;
            selfish[2]   = 0;
        }
    }

    // Initialization of the random numbers generator
    RND_Init(S, rnd_file, LPID);

    // User level handler to get some configuration parameters from the runtime environment
    // (e.g. the GAIA parameters and many others)
    user_environment_handler();

    generate_hashrates(NSIMULATE);

    /*
     *      Set-up of the GAIA framework
     *
     *      Parameters:
     *      1. (SIMULATE*NLP)   Total number of simulated entities
     *      2. (NLP)	    Number of LPs in the simulation
     *      3. (rnd_file)       Seeds file for the random numbers generator
     *      4. (NULL)           LP canonical name
     *      5. (SIMA_HOST)	    Hostname where the SImulation MAnager is running
     *      6. (SIMA_PORT)	    SIMA TCP port number
     */
    LPID = GAIA_Initialize(NSIMULATE * NLP, NLP, rnd_file, NULL, SIMA_HOST, SIMA_PORT);

    // Returns the length of the timestep
    // this value is defined in the "CHANNELS.TXT" configuration file
    // given that GAIA is based on the time-stepped synchronization algorithm
    // it retuns the size of a step
    step = GAIA_GetStep();

    // Due to synchronization constraints The FLIGHT_TIME has to be bigger than the timestep size
    if (FLIGHT_TIME < step) {
        fprintf(stdout, "FATAL ERROR, the FLIGHT_TIME (%8.2f) is less than the timestep size (%8.2f)\n", FLIGHT_TIME, step);
        fflush(stdout);
        exit(-1);
    }

    // First identifier (ID) of SEs allocated in the local LP
    start = NSIMULATE * LPID;

    // Number of SEs to allocate in the local LP
    count = NSIMULATE;

    //  Used to set the ID of the first simulated entity (SE) in the local LPnsimulnsimulnsimul
    GAIA_SetFstID(start);

    // Output file for statistics (communication ratio data)
    dat_filename = malloc(1024);
    snprintf(dat_filename, 1024, "%stmp-evaluation-lcr.dat", TESTNAME);
    lcr_fp = fopen(dat_filename, "w");

    // Data structures initialization (hash tables and migration list)
    hash_init(table, NSIMULATE * NLP);                  // Global hashtable: all the SEs
    hash_init(stable, NSIMULATE);                       // Local hastable: local SEs
    list_init(mlist);                                   // Migration list (pending migrations in the local LP)

    // Starting the execution timer
    TIMER_NOW(t1);

    fprintf(stdout, "#LP [%d] HOSTNAME [%s]\n", LPID, LP_HOST);
    fprintf(stdout, "#                      LP[%d] STARTED\n#\n", LPID);
    fprintf(stdout, "#          Generating Simulated Entities from %d To %d ... ", (LPID * NSIMULATE), ((LPID * NSIMULATE) + NSIMULATE) - 1);
    fflush(stdout);

    // Generate all the SEs managed in this LP
    Generate(count);
    fprintf(stdout, " OK\n#\n");

    fprintf(stdout, "# Data format:\n");
    fprintf(stdout, "#\tcolumn 1:	elapsed time (seconds)\n");
    fprintf(stdout, "#\tcolumn 2:	timestep\n");
    fprintf(stdout, "#\tcolumn 3:	number of entities in this LP\n");
    fprintf(stdout, "#\tcolumn 4:	number of migrating entities (from this LP)\n");

    // It is the LP that manages statistics
    if (LPID == LP_STAT) {                      // Verbose output
        fprintf(stdout, "#\tcolumn 5:	local communication ratio (percentage)\n");
        fprintf(stdout, "#\tcolumn 6:	remote communication ratio (percentage)\n");
        fprintf(stdout, "#\tcolumn 7:	total number of migrations in this timestep\n");
    }
    fprintf(stdout, "#\n");

    // Dynamically allocating some space to receive messages
    data = malloc(BUFFER_SIZE);
    ASSERT((data != NULL), ("simulation main: malloc error, receiving buffer NOT allocated!"));

    // Before starting the real simulation tasks, the model level can initialize some
    //	data structures and set parameters
    user_bootstrap_handler();

    /* Main simulation loop, receives messages and calls the handler associated with them */
    while (!end_reached) {
        // Max size of the next message.
        //  after the receive the variable will contain the real size of the message
        max_data = BUFFER_SIZE;

        // Looking for a new incoming message
        msg_type = GAIA_Receive(&from, &to, &Ts, (void *)data, &max_data);
        msg      = (Msg *)data;

        // A message has been received, process it (calling appropriate handler)
        //  message handlers
        switch (msg_type) {
        // The migration of a locally managed SE has to be done,
        //	calling the appropriate handler to insert the SE identifier
        //	in the list of pending migrations
        case NOTIF_MIGR:
            notify_migration_event_handler(from, to);
            break;

        // A migration has been executed in the simulation but the local
        //	LP is not directly involved in the migration execution
        case NOTIF_MIGR_EXT:
            notify_ext_migration_event_handler(from, to);
            break;

        // Registration of a new SE that is managed by another LP
        case REGISTER:
            register_event_handler(from, to);
            break;

        // The local LP is the receiver of a migration and therefore a new
        //	SE has to be managed in this LP. The handler is responsible
        //	to allocate the necessary space in the LP data structures
        //	and in the following to copy the SE state that is contained
        //	in the migration message
        case EXEC_MIGR:
            migration_event_handler(from, msg);
            break;

        // End Of Step:
        //	the current simulation step is finished, some pending operations
        //	have to be performed
        case EOS:
            // Stopping the execution timer
            //	(to record the execution time of each timestep)
            TIMER_NOW(t2);

            /*  Actions to be done at the end of each simulated timestep  */
            if (simclock < env_end_clock) { // The simulation is not finished
                // Simulating the interactions among SEs
                //
                //	in the last (env_end_clock - FLIGHT_TIME) timesteps
                //	no msgs will be sent because we wanna check if all
                //	sent msgs are correctly received
                if (simclock < (env_end_clock - FLIGHT_TIME)) {
                    Generate_Computation_and_Interactions(NSIMULATE * NLP);
                }

                // The pending migration of "flagged" SEs has to be executed,
                //	the SE to be migrated were previously inserted in the migration
                //	list due to the receiving of a "NOTIF_MIGR" message sent by
                //	the GAIA framework
                //migrated_in_this_step = ScanMigrating();

                // The LP that manages statistics prints out them
                if (LPID == LP_STAT) {                                  // Verbose output
                    // Some of them are provided by the GAIA framework
                    GAIA_GetStatistics(&loc, &rem, &migr);

                    // Total number of migrations (in the simulation run)
                    tot += migr;

                    // Printed fields:
                    //  elapsed Wall-Clock-Time up to this step
                    //	timestep number
                    //	number of entities in this LP
                    //	percentage of local communications (intra-LP)
                    //	percentage of remote communications (inter-LP)
                    //	number of migrations in this timestep

                    // Total number of interactions (in the timestep)
                    #ifdef DEBUG
                    float t = loc + rem;
                    fprintf(stdout, "- [%11.2f]\t[%6.5f]\t%4.0f\t%2.2f\t%2.2f\t%d\n", TIMER_DIFF(t2, t1), simclock, (float)stable->count, (float)loc / (float)t * 100.0, (float)rem / (float)t * 100.0, migr);
                    if (simclock >= 7) { fprintf(lcr_fp, "%f\n", (float)loc / (float)t * 100.0); }
                    #endif
                }else {
                    // Reduced output
                    #ifdef DEBUG
                    fprintf(stdout, "[%11.2fs]   %12.2f [%d]\n", TIMER_DIFF(t2, t1), simclock, stable->count);
                    #endif
                }

                // Now it is possible to advance to the next timestep
                simclock = GAIA_TimeAdvance();
            }else {
                /* End of simulation */
                TIMER_NOW(t2);

                fprintf(stdout, "\n\n");
                fprintf(stdout, "### Termination condition reached (%d)\n", tot);
                fprintf(stdout, "### Clock           %12.2f\n", simclock);
                fprintf(stdout, "### Elapsed Time    %11.2fs\n", TIMER_DIFF(t2, t1));
                fprintf(stdout, "### Total sent txs: %10ld; Total received txs: %10ld\n", get_total_sent_trans(), get_total_received_trans());
                fprintf(stdout, "### Total sent blocks: %10ld; Total received blocks: %10ld\n", get_total_sent_blocks(), get_total_received_blocks());
                fflush(stdout);

                end_reached = 1;
            }
            break;

        // Simulated model events (user level events)
        case UNSET:
            // First some checks for validation
            tmp_node = validation_model_events(from, to, msg);

            // The appropriate handler is defined at model level
            user_model_events_handler(to, from, msg, tmp_node);
            break;

        default:
            fprintf(stdout, "FATAL ERROR, received an unknown event type: %d\n", msg_type);
            fflush(stdout);
            exit(-1);
        }
    }

    // Finalize the GAIA framework
    GAIA_Finalize();

    // Before shutting down, the model layer is able to deallocate some data structures
    user_shutdown_handler();

    // Closing output file for performance evaluation
    fclose(lcr_fp);

    // Freeing of the receiving buffer
    free(data);

    // Creating the "finished file" that is used by some scripts
    tmp_filename = malloc(256);
    snprintf(tmp_filename, 256, "%d.finished", LPID);
    finished_fp = fopen(tmp_filename, "w");
    fclose(finished_fp);

    // That's all folks.
    return(0);
}
