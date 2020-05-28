/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              For a general introduction to LUNES implmentation please see the
 *              file: mig-agents.c
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

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
#include <rnd.h>
#include <values.h>
#include "utils.h"
#include "user_event_handlers.h"
#include "lunes.h"
#include "lunes_constants.h"
#include "entity_definition.h"


/* ************************************************************************ */
/*       L O C A L	V A R I A B L E S			                            */
/* ************************************************************************ */

FILE *         fp_print_trace;        // File descriptor for simulation trace file
unsigned short env_max_ttl = MAX_TTL; // TTL of newly created messages


/* ************************************************************************ */
/*          E X T E R N A L     V A R I A B L E S                           */
/* ************************************************************************ */

extern hash_t hash_table, *table;                   /* Global hash table of simulated entities */
extern hash_t sim_table, *stable;                   /* Hash table of locally simulated entities */
extern double simclock;                             /* Time management, simulated time */
extern TSeed  Seed, *S;                             /* Seed used for the random generator */
extern char * TESTNAME;                             /* Test name */
extern int    NSIMULATE;                            /* Number of Interacting Agents (Simulated Entities) per LP */
extern int    NLP;                                  /* Number of Logical Processes */
// Simulation control
extern unsigned short env_dissemination_mode;       /* Dissemination mode */
extern float          env_broadcast_prob_threshold; /* Dissemination: conditional broadcast, probability threshold */
extern unsigned int   env_cache_size;               /* Cache size of each node */
extern float          env_freerider_prob;           /* Probability that a given node is a free-rider */
extern float          env_fixed_prob_threshold;     /* Dissemination: fixed probability, probability threshold */
extern float          env_dandelion_fluff_steps;    /* Dissemination: dandelion, number of stem steps */
#ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
extern unsigned int env_probability_function;       /* Probability function for Degree Dependent Gossip */
extern double       env_function_coefficient;       /* Coefficient of the probability function */
#endif
extern float  env_global_hashrate;                  /* Total Hashrate of Bitcoin Network in H/min */
extern double env_difficulty;                       /* Actual blockchain network difficulty */
extern int *  selfish;                              /* Used for the selfish mining */
extern int number_of_heads;                         /* Number of forks is kept track in the system. The head is the last block of a chain */
extern int number_dos_nodes;                        /* dos attackers that don't forward victim messages  */
extern int victim;
extern unsigned short env_max_ttl;                  /* TTL of new messages */
int dand_plus_waiting = 5;
int actual_dos_nodes;
/* ************************************************************************ */
/*          S U P P O R T      F U N C T I O N S		                    */
/* ************************************************************************ */

#ifdef FORKING                                                //Following methods in case the forking version is used


/*if the block with the given ID is a head block of a fork his index (of the heads list) is return; otherwise -1 is returned
 *
 *  @param[in] heads: array of pointer to head-blocks
 *  @param[in] id: id of the block to be found
 */
int is_in_heads (Block ** heads , int id){
	int ret = -1;
	for (int i =0; i<number_of_heads; i++){    
        if (heads[i] != NULL){                  
		    if (heads[i]->id == id){
		        ret = i;
		      	break;
		    }
        }
        else {
            break;
        }
	}
	return ret;
}


/*if the block with the given ID is in the blockchain his index (of the blocks list) is return; otherwise -1 is returned
 *
 *  @param[in] blockchain: Pointer to the blockchain
 *  @param[in] id: id whose index has to be found
 *  @param[in] latest: index of the latest element inserted in the blockchain
 */
int getIndexById (Block* blockchain, int latest, int id){
	int res = -1;
	for (int i= latest; i >= 0; i--){                                     //starting from the latest index in the blockchain
    	if (blockchain[i].id == id){
    		res = i;
    		break;
    	}
    }
    return res;
}


// A new head block is declared.
void replace_heads (Block ** heads, int old, Block * newBlock ){
    heads[old] = newBlock;
}


/*once you find a block whose position is greater than parameter position, the block and his index in the blocks list is returned
 *
 *  @param[in] blockchain: Pointer to the blockchain
 *  @param[in] from: index to start the iteration from
 *  @param[in] latest: index of the latest element inserted in the blockchain
 *  @param[in] position: looking for blocks whose position is >= than this int
 */
tTuple find_block_given_position (Block* blockchain, int from, int latest, int position){
	Block * given_block = NULL;
    int i;
    for (i= from; i <= latest; i++){
    	if (blockchain[i].position >= position){
    		given_block = &blockchain[i];
    		break;
    	}
    }
	tTuple res = {given_block, i};
    return res;
}


/*the index of the head-block with the greater position is returned
 *
 *  @param[in] heads: array of pointers to head-blocks
 */
int heads_greater_position (Block ** heads){
	int max = -1;
	int res = -1;
	for (int i = 0; i<number_of_heads; i++){
        if (heads[i] != NULL){
            if (heads[i]->position > max && heads[i]->position > 0){
        	   max = heads[i]->position;
        	   res = i;
            }
        } 
        else {
            break;
        }       
	}
	return res;
}

/*Given an id of a block, the index of the next block is returned
 *
 *  @param[in] blockchain: Pointer to the blockchain
 *  @param[in] id: id whose index of the next block has to be found
 *  @param[in] latest: index of the latest element inserted in the blockchain
 */
int is_next_in_blockchain (Block * blockchain, int latest, int id){
	int ret = -1;
	for (int i = 0; i < latest; i++){
		if (blockchain[i].prevId == id){
			ret = i;
		    break;
		}
	}
	return ret;
}


/*The smallest fork is replaced with a new one, is not kept track anymore of the head-block of the 11th longest chain
 *
 *  @param[in] heads: array of pointers to head-blocks
 *  @param[in] newBlock: candidate to become a new head-block
 */
void add_heads (Block ** heads, Block * newBlock ){
	int del = minimum_pos(heads);
    if (heads[del] != NULL){
	   if (heads[del]->position < newBlock->position){
    	   replace_heads(heads, del, newBlock);
        }
    }else{ 
        replace_heads(heads, del, newBlock);
    }
}
/*return the index (in the heads list) of the head block with the smallest position
 *
 *  @param[in] heads: array of pointers to head-blocks
 */
int minimum_pos (Block ** heads){
	int	position = 0;
	int	minimum = MAXINT;
	int	tmp;

	for (tmp = 0; tmp < number_of_heads; tmp++){
        if (heads[tmp] == NULL){
            return tmp;
        } else {
		    if (heads[tmp]->position < minimum) {
		      	minimum = heads[tmp]->position;
			    position = tmp;
            }
		}
    }
	return(position);
}
/*If a block is the blockchain the index of the blocks list is returned, otherwise -1 is returned
 *
 *  @param[in] blockchain: Pointer to the blockchain
 *  @param[in] id: id to find
 *  @param[in] latest: index of the latest element inserted in the blockchain
 */
int is_in_blockchain (Block * blockchain, int id, int latest){
	int ret = -1;
	for (int i =0; i < latest; i++){                   
		if (blockchain[i].id == id){
			ret = i;
			break;
		}
	}
	return ret;
}

#endif


#ifdef TXDEBUG

/*! \brief Support function to print the entire blockchain of a node
 *
 * @param[in] b: Pointer to the node's blockchain
 * @param[in] n: Latest block
 * @param[in] nodeid: ID of the node
 */
void print_blockchain(Block *b, int n, int nodeid) {
    fprintf(stdout, "BLOCKCHAIN NODE %d\n", nodeid);
    for (int i = 0; i <= n; ++i) {
        fprintf(stdout, "BLOCK: %d, lttrans: %d\n", i, b[i].latesttrans);
        for (int t = 0; t < b[i].latesttrans; ++t) {
            fprintf(stdout, "\t BLOCK: %d, txid: %d\n", i, b[i].trans[t].id);
        }
    }
    fprintf(stdout, "END BLOCKCHAIN NODE %d\n", nodeid);
    fflush(stdout);
}

/*! \brief Inserts a new transaction from the network inside the latest block
 *
 *  @param[in] b: Pointer to the node's blockchain
 *  @param[in] n: Index of latest block
 *  @param[in] from: Id of the sender (fake id)
 *  @param[in] to: ID of the receiver (fake id)
 *  @param[in] id: ID of the transaction (fake id)
 */
void lunes_trans_insert(Block *b, int n, int from, int to, int id) {
    int num = b[n].latesttrans;

    if (num < 1500 - 1) {
        b[n].latesttrans   = num + 1;
        b[n].trans[num].id = id;
    }
}

/*! \brief Boolean, verifies if the received transaction is already in the local cache of the node
 *
 *  @param[in] b: Pointer to the node's blockchain
 *  @param[in] n: Index of latest block
 *  @param[in] id: ID of the transaction
 *  @return True or False
 */
int lunes_trans_is_known(Block *b, int n, int id) {
    int retvalue = 0;
    int tmp;

    for (int i = 0; i <= n && retvalue == 0; ++i) {
        for (tmp = 0; tmp < b[i].latesttrans; tmp++) {
            if (b[i].trans[tmp].id == id) {
                retvalue = 1;
                break;
            }
        }
    }
    return(retvalue);
}

#endif

/*! \brief Used to calculate the forwarding probability value for a given node
 */
#ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
double lunes_degdependent_prob(unsigned int deg) {
    double prob = 0.0;

    switch (env_probability_function) {
    // Function 2
    case 2:
        prob = 1.0 / log(env_function_coefficient * deg);
        break;

    // Function 1
    case 1:
        prob = 1.0 / pow(deg, env_function_coefficient);
        break;

    default:
        fprintf(stdout, "%12.2f FATAL ERROR, function: %d does NOT exist!\n", simclock, env_probability_function);
        fflush(stdout);
        exit(-1);
        break;
    }

    /*
     * The probability is defined in the range [0, 1], to get full dissemination some functions require that the negative
     * results are treated as "true" (i.e. 1)
     */
    if ((prob < 0) || (prob > 1)) {
        prob = 1;
    }
    return(prob);
}

#endif

/*! \brief Used to forward a received message to all (or some of)
 *         the neighbors of a given node. BlockMsg have max priority
 *         so all node will broadcast this messages.
 *         Transactions are regulated by the dissemination protocol
 *         implementation.
 *
 *  @param[in] node: The node doing the forwarding
 *  @param[in] msg: Message to forward
 *  @param[in] ttl: TTL of the message
 *  @param[in] timestamp: Timestamp of message's arrival
 *  @param[in] creator: Node sender
 *  @param[in] forwarder: Agent forwarder
 */
void lunes_real_forward(hash_node_t *node, Msg *msg, unsigned short ttl, float timestamp, unsigned int creator, unsigned int forwarder) {
    // Iterator to scan the whole state hashtable of neighbors
    GHashTableIter iter;
    gpointer       key, destination;
    float          threshold;         // Tmp, used for probabilistic-based dissemination algorithms
    hash_node_t *  sender, *receiver; // Sender and receiver nodes in the global hashtable
    int            txid, from = 2, to = 1;

    switch (msg->type) {
    // BlockMsg is always disseminated to all neighbors
    case 'B':
        g_hash_table_iter_init(&iter, node->data->state);
        // All neighbors
        while (g_hash_table_iter_next(&iter, &key, &destination)) {
            sender   = hash_lookup(stable, node->data->key);             // This node
            receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

            // The original forwarder of this message and its creator are exclueded
            // from this dissemination
            if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                execute_block(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->block.block_static.minedblock, timestamp, creator);
            }
        }
        break;

    // TransMsg is regulated by the dissemination choise
    case 'T':
        // Dissemination mode for the forwarded messages (dissemination algorithm)
        switch (env_dissemination_mode) {
        case BROADCAST:
            // The message is forwarded to ALL neighbors of this node
            // NOTE: in case of probabilistic broadcast dissemination this function is called
            //		 only if the probabilities evaluation was positive
            g_hash_table_iter_init(&iter, node->data->state);

            // All neighbors
            while (g_hash_table_iter_next(&iter, &key, &destination)) {
                sender   = hash_lookup(stable, node->data->key);             // This node
                receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

                // The original forwarder of this message and its creator are exclueded
                // from this dissemination
                if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                    txid = msg->trans.trans_static.transid;
                    execute_trans(simclock + FLIGHT_TIME, sender, receiver, ttl, txid, from, to, timestamp, creator);
                }
            }
            break;

        case DANDELION:
        case DANDELIONPLUS:
            g_hash_table_iter_init (&iter, node->data->state);
            txid = msg->trans.trans_static.transid;
            if (ttl >= env_dandelion_fluff_steps ){                   //stem phase
                int neighbors = node->data->num_neighbors;
                int selectedIndex = RND_Interval(S, 0, neighbors - 1);            //just sending the message to the neighbor of that index
                int iterIndex = 0;
                while (g_hash_table_iter_next (&iter, &key, &destination)) {

                    sender = hash_lookup(stable, node->data->key);                      // This node
                    receiver = hash_lookup(table, *(unsigned int *)destination);        // The neighbor

                    if (iterIndex == selectedIndex){                 //message just sent to the neighbor of index selectedIndex, generated randomly
                        execute_trans (simclock + FLIGHT_TIME, sender, receiver, ttl, txid, from, to, timestamp, creator);
                        break;
                    }
                    iterIndex++;
                }
            } else {                                                                //fluff phase, sending messages to everyone, except the forwarder
                while (g_hash_table_iter_next (&iter, &key, &destination)) {

                    sender = hash_lookup(stable, node->data->key);                  // This node
                    receiver = hash_lookup(table, *(unsigned int *)destination);    // The neighbor

                    if (receiver->data->key != forwarder )
                        execute_trans (simclock + FLIGHT_TIME, sender, receiver, ttl, txid, from, to, timestamp, creator);
                }
            }
        break;

        case GOSSIP_FIXED_PROB:
            // In this case, all neighbors will be analyzed but the message will be
            // forwarded only to some of them
            g_hash_table_iter_init(&iter, node->data->state);

            // All neighbors
            while (g_hash_table_iter_next(&iter, &key, &destination)) {
                // Probabilistic evaluation
                threshold = RND_Interval(S, (double)0, (double)100);

                if (threshold <= env_fixed_prob_threshold) {
                    sender   = hash_lookup(stable, node->data->key);             // This node
                    receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

                    // The original forwarder of this message and its creator are exclueded
                    // from this dissemination
                    if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                        txid = msg->trans.trans_static.transid;
                        execute_trans(simclock + FLIGHT_TIME, sender, receiver, ttl, txid, from, to, timestamp, creator);
                    }
                }
            }
            break;

        #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
        // Degree Dependent dissemination algorithm
        case DEGREE_DEPENDENT_GOSSIP:
            g_hash_table_iter_init(&iter, node->data->state);

            // All neighbors
            while (g_hash_table_iter_next(&iter, &key, &destination)) {
                sender   = hash_lookup(stable, node->data->key);             // This node
                receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

                // The original forwarder of this message and its creator are excluded
                // from this dissemination
                if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                    // Probabilistic evaluation
                    threshold = (RND_Interval(S, (double)0, (double)100)) / 100;

                    // If the eligible recipient has less than 3 neighbors, its reception probability is 1. However,
                    // if its value of num_neighbors is 0, it means that I don't know the dimension of
                    // that node's neighborhood, so the threshold is set to 1/n, being n
                    // the dimension of my neighborhood
                    if (((value_element *)destination)->num_neighbors < 3) {
                        // Note that, the startup phase (when the number of neighbors is not known) falls in
                        // this case (num_neighbors = 0)
                        // -> full dissemination
                        txid = msg->trans.trans_static.transid;
                        execute_trans(simclock + FLIGHT_TIME, sender, receiver, ttl, txid, from, to, timestamp, creator);
                    }
                    // Otherwise, the probability is evaluated according to the function defined by the
                    // environment variable env_probability_function
                    else{
                        if (threshold <= lunes_degdependent_prob(((value_element *)destination)->num_neighbors)) {
                            txid = msg->trans.trans_static.transid;
                            execute_trans(simclock + FLIGHT_TIME, sender, receiver, ttl, txid, from, to, timestamp, creator);
                        }
                    }
                }
            }
            break;
            #endif

        default:
            fprintf(stdout,
                    "%12.2f FATAL ERROR, the dissemination mode [%2d] is NOT implemented in this version of LUNES!!!\n",
                    simclock,
                    env_dissemination_mode);
            fprintf(stdout,
                    "%12.2f NOTE: all the adaptive protocols require compile time support: see the ADAPTIVE_GOSSIP_SUPPORT define in sim-parameters.h\n",
                    simclock);
            fflush(stdout);
            exit(-1);
            break;
        }
        break;
    }
}

/*! \brief Dissemination protocol implementation.
 *         A new message has been received from a neighbor,
 *         it is necessary to forward it in some way
 *
 *  @param[in] node: The node doing the forwarding
 *  @param[in] msg: Message to forward
 *  @param[in] ttl: TTL of the message
 *  @param[in] timestamp: Timestamp of message's arrival
 *  @param[in] creator: Node sender
 *  @param[in] forwarder: Agent forwarder
 */
void lunes_forward_to_neighbors(hash_node_t *node, Msg *msg, unsigned short ttl, float timestamp, unsigned int creator, unsigned int forwarder) {
    float threshold; // Tmp, probabilistic evaluation

    // Dissemination mode for the forwarded messages
    switch (env_dissemination_mode) {
    case BROADCAST:
        // Probabilistic evaluation
        threshold = RND_Interval(S, (double)0, (double)100);

        if (threshold <= env_broadcast_prob_threshold) {
            lunes_real_forward(node, msg, ttl, timestamp, creator, forwarder);
        }
        break;

    case GOSSIP_FIXED_PROB:      
    case DANDELION:
    case DANDELIONPLUS:
        lunes_real_forward(node, msg, ttl, timestamp, creator, forwarder);
        break;

    #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
    case DEGREE_DEPENDENT_GOSSIP:
        lunes_real_forward(node, msg, ttl, timestamp, creator, forwarder);
        break;
    #endif

    default:
        fprintf(stdout, "%12.2f FATAL ERROR, the dissemination mode [%2d] is NOT implemented in this version of LUNES!!!\n", simclock, env_dissemination_mode);
        fprintf(stdout, "%12.2f NOTE: all the adaptive protocols require compile time support: see the ADAPTIVE_GOSSIP_SUPPORT define in sim-parameters.h\n", simclock);
        fflush(stdout);
        exit(-1);
        break;
    }
}

/*! \brief Dissemination protocol implementation for transactions.
 *         A new message has been generated in this node
 *         and now it is propagated to (some) neighbors
 *
 * @param[in] node: The node doing the sending
 * @param[in] txid: ID of the transaction
 * @param[in] from: ID of the sender (fake ID)
 * @param[in] to: ID of the receiver (fake ID)
 */
void lunes_send_trans_to_neighbors(hash_node_t *node, int txid, int from, int to) {
    // Iterator to scan the whole state hashtable of neighbors
    GHashTableIter iter; 
    gpointer       key, destination;

    // All neighbors
    g_hash_table_iter_init(&iter, node->data->state);

    while (g_hash_table_iter_next(&iter, &key, &destination)) {
        // It's a standard trans message
        execute_trans(simclock + FLIGHT_TIME, hash_lookup(stable, node->data->key), hash_lookup(table, *(unsigned int *)destination), env_dandelion_fluff_steps - 1, txid, from, to, simclock, node->data->key);
    }
}

/*! \brief Dissemination protocol implementation for mined blocks.
 *         A new block has been mined in this node
 *         and now it is propagated to (some) neighbors
 *
 * @param[in] node: The node doing the sending
 * @param[in] b: Block to forward
 */
void lunes_send_block_to_neighbors(hash_node_t *node, Block *b) {
    // Iterator to scan the whole state hashtable of neighbors
    GHashTableIter iter;
    gpointer       key, destination;

    // All neighbors
    g_hash_table_iter_init(&iter, node->data->state);

    while (g_hash_table_iter_next(&iter, &key, &destination)) {
        // It's a standard trans message
        execute_block(simclock + FLIGHT_TIME, hash_lookup(stable, node->data->key), hash_lookup(table, *(unsigned int *)destination), env_max_ttl, b, simclock, node->data->key);
    }
}

/*! \brief Dissemination protocol implementation for mined blocks.
 *         A new block has been mined in this node
 *         and now it is propagated to (some) neighbors
 *
 * @param[in] node: The node doing the sending
 * @param[in] b: Blockchain pointer
 * @param[in] dest: Node recipient
 */
void lunes_responde_ask_block(hash_node_t *node, Block *b, int dest) {
    BlockMsg     msg;
    unsigned int message_size;

    // Defining the message type
    msg.block_static.type = 'B';

    msg.block_static.timestamp  = simclock;
    msg.block_static.ttl        = 2;
    msg.block_static.minedblock = b;
    msg.block_static.creator    = node->data->key;

    // To reduce the network overhead, only the used part of the message is really sent
    message_size = sizeof(struct _block_static_part);

    // Buffer checkABRABR
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }

    #ifdef ASKBLOCKDEBUG
    // clock - nodeid - blockid - tonode
    fprintf(stdout, "ABR: %.0f,%d,%d,%d,%d\n", simclock, node->data->key, b->id, dest, b->position);
    #endif

    // Real send
    GAIA_Send(node->data->key, dest, simclock + FLIGHT_TIME, (void *)&msg, message_size);
}

/* -----------------------   GRAPHVIZ DOT FILES SUPPORT --------------------- */

/*! \brief Support function for the parsing of graphviz dot files,
 *         used for loading the graphs (i.e. network topology)
 */
void lunes_dot_tokenizer(char *buffer, int *source, int *destination) {
    char *token;
    int   i = 0;

    token = strtok(buffer, "--");
    do {
        i++;

        if (i == 1) {
            *source = atoi(token);
        }

        if (i == 2) {
            token[strlen(token) - 1] = '\0';
            *destination             = atoi(token);
        }
    } while ((token = strtok(NULL, "--")));
}

/*! \brief Parsing of graphviz dot files,
 *         used for loading the graphs (i.e. network topology)
 */
void lunes_load_graph_topology() {
    FILE *dot_file;
    char  buffer[1024];
    int   source      = 0,
          destination = 0;
    hash_node_t *source_node,
                *destination_node;
    value_element val;
    // What's the file to read?
    sprintf(buffer, "%s%s", TESTNAME, TOPOLOGY_GRAPH_FILE);
    dot_file = fopen(buffer, "r");

    // Reading all of it
    while (fgets(buffer, 1024, dot_file) != NULL) {
        // Parsing line by line
        lunes_dot_tokenizer(buffer, &source, &destination);

        // I check all the edges defined in the dot file to build up "link messages"
        // between simulated entities in the simulated network model

        // Is the source node a valid simulated entity?
        if ((source_node = hash_lookup(stable, source)))  {
            // Is destination vertex a valid simulated entity?
            if ((destination_node = hash_lookup(table, destination))) {
                #ifdef AG_DEBUG
                fprintf(stdout, "%12.2f node: [%5d] adding link to [%5d]\n", simclock, source_node->data->key, destination_node->data->key);
                #endif

                // Creating a link between simulated entities (i.e. sending a "link message" between them)
                execute_link(simclock + FLIGHT_TIME, source_node, destination_node);

                // Initializing the extra data for the new neighbor
                val.value = destination;

                // I've to insert the new link (and its extra data) in the neighbor table of this sender,
                // the receiver will do the same when receiving the "link request" message

                // Adding a new entry in the local state of the sender
                //	first entry	= key
                //	second entry	= value
                //	note: no duplicates are allowed
                if (add_entity_state_entry(destination, &val, source, source_node) == -1) {
                    // Insertion aborted, the key is already in the hash table
                    fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] key %d (value %d) is a duplicate and can not be inserted in the hash table of local state\n", simclock, source, destination, destination);
                    fflush(stdout);
                    exit(-1);
                }
            }else {
                fprintf(stdout, "%12.2f FATAL ERROR, destination: %d does NOT exist!\n", simclock, destination);
                fflush(stdout);
                exit(-1);
            }
        }
    }

    fclose(dot_file);
}

/* ************************************************************************ */
/*  L U N E S     U S E R    L E V E L     H A N D L E R S		            */
/* ************************************************************************ */

/*! \brief Calculate the probability of mining a block in time 'time_min' minutes with
 *         an hashrate percentage 'hashrate' of the total hashrate
 */
double mining_probability(double hashrate, double time_min) {
    int    t     = (int)time_min % 10;
    double bonus = 0.0,
           malus = 0.0;

    t = t == 0 ? 10 : t;
    // Needed if the actual network is in steady state
    if (t >= 8) {
    }else {
    }
    return((((hashrate * env_global_hashrate / 100.0) * t) / (HASHPOW * env_difficulty)) + bonus - malus);
}

/****************************************************************************
 *! \brief LUNES_CONTROL: node activity for the current timestep
 * @param[in] node: Node that execute actions
 */
void lunes_user_control_handler(hash_node_t *node) {

    if (simclock == INTERMEDIATE_STEPS){
		GHashTableIter iter;
	    gpointer       key, destination;
	    g_hash_table_iter_init(&iter, node->data->state);
	    int count = 0;
	        // All neighbors
	    while (g_hash_table_iter_next(&iter, &key, &destination)) {
	        count = count +1;
	    }
	    node->data->num_neighbors = count;
    }
    if ((int) simclock % INTERMEDIATE_STEPS == 0) {
	    int ltblock = node->data->latestblock;    //index of latest block

	    // Check if the node has mined this block
	    if (node->data->miner) {
	        double random_mine = RND_Interval(S, 0.0, 1.0);
	        double p           = mining_probability(node->data->hashrate, simclock);
	        // Avoid luck
	        if (p > random_mine && node->data->internal_timer > 2) {
	        	#ifdef FORKING
                Block *b = &node->data->s_state.blockchain[ltblock];
	            int newId = RND_Interval(S, 0, MAXINT - 1);
		        if (ltblock < 2500 - 1){    //with rispect to the non-forking version the blockchain size is increased, due to foking version producing more blocks
		            b->id = newId;
		            int headPos= heads_greater_position(node->data->s_state.heads);   //we want to continue the longest chain so we get its head block
		            if ( headPos >= 0){                                               //adding the block to the longest chain
                        Block **  heads= node->data->s_state.heads;                   //array of pointers to head-blocks
                        Block * thatBlock = heads[headPos];                           //head block of the longest chain
		               	b->prevId = thatBlock->id;                                    //continuing the longest chain
		               	b->position = thatBlock->position + 1;                        //position = position of the previous node + 1
		               	replace_heads (node->data->s_state.heads, headPos, b);        //update the main chain. The head is now the new mined node
		            } else {                //in case still no chain exists
		            	b->prevId = -1;                                               //first block of a chain has -1 as previous block index 
                        b-> position = 1;                                             //and 1 as position             
		                replace_heads (node->data->s_state.heads, 0, b);              //new block now replaces its previous block as head-block
		            }
		            node->data->latestblock += 1;                                     //number of blocks memorized in the blockchain increased by 1
	            }

	            #else
	            if (ltblock < 1500 - 1) {
	                // Point to the next block
	                node->data->latestblock += 1;
	                // Update the ID of the mined block
	                node->data->s_state.blockchain[ltblock].id = ltblock;
	            }
	            #endif 

	            node->data->internal_timer = 0;
	            if (node->data->attackerid == node->data->key && selfish[2] >= 0) {
	                // clock - nodeid - blockminedid - hashrate
	                #ifdef FORKING   
	                //underscore characted added in order to facilitate the recognition of the IDs
	                fprintf(stdout, "BS: %.0f,%d,_%d,%d,%f,%d\n", simclock, node->data->key, b->id, b->prevId, node->data->hashrate, b-> position);
	                //fprintf(stdout, "BSS: %.0f,%d,%d,%d,%f\n", simclock, node->data->key, b->id, b->position, node->data->hashrate);
	                #else
	                fprintf(stdout, "BSS: %.0f,%d,%d,%f\n", simclock, node->data->key, ltblock, node->data->hashrate);
	                #endif
	                // Update private chain
	                #ifdef FORKING
	                selfish[0] = newId;
	                #else
	                selfish[0]  = ltblock;
	                #endif
	                selfish[2] += 1;
	                if (selfish[2] >= 2) {              //2 nodes in advantages, then propagating the mined nodes through the network
	                                                    //Head block and his previous block are spread through the network. 
	                    #ifdef FORKING                  //in FORKING mode the indexes in the blockchain have to be found
	                    fprintf(stdout, "possible selfish successful %d %d %d\n", selfish[0], selfish[1], selfish[2]);
	                    int ind = getIndexById (&node->data->s_state.blockchain[0], node->data->latestblock, newId);
	                    int prev = getIndexById (&node->data->s_state.blockchain[0], node->data->latestblock, node->data->s_state.blockchain[ind].prevId);
	                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[prev]);
	                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[ind]);
	                    #else
	                    fprintf(stdout, "possible selfish successful %d %d %d\n", selfish[0], selfish[1], selfish[2]);
	                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[ltblock - 1]);
	                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[ltblock]);
	                    #endif
	                    // Reset the count
	                    selfish[2] = 0;
	                }
	            }else {
	                // Broadcasting the mined ("old") block to all neighbors
                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[ltblock]);
	                // clock - nodeid - blockminedid - hashrate
	                #ifdef FORKING
	                fprintf(stdout, "BS: %.0f,%d,_%d,%d,%f,%d\n", simclock, node->data->key, newId, b->prevId, node->data->hashrate, b-> position);
	                #else
	                fprintf(stdout, "BS: %.0f,%d,%d,%f\n", simclock, node->data->key, ltblock, node->data->hashrate);
	                #endif
	            }
	            fflush(stdout);
	        }else {
	            node->data->internal_timer += 1;
	        }
	    }
	    // If the timer expires we can proceed to the generation of the new message
	    // but this will happen only for nodes that are enabled to the generation of
	    // new messages (e.g. time_of_next_trans == -1 -> this node is a forwarder)
	    // avoid transaction spam using a probability (precision: 0.995153)
	    #ifdef TXDEBUG
	    if (((node->data->s_state.time_of_next_trans >= 0) &&
	         (node->data->s_state.time_of_next_trans <= simclock)) || node->data->key == 337) {
	        // If the node is the victim node generate always a transaction
	        // Reset of the timer, it is the time of the next sending
	        node->data->s_state.time_of_next_trans = simclock + (RND_Exponential(S, 1) * MEAN_NEW_MESSAGE * INTERMEDIATE_STEPS);

	        // Creating a (maybe) unique identifier for the new message
	        int transactionid = RND_Interval(S, 0, MAXINT - 1);
	        // This two fields are unused to reduce ram usage
	        int from = node->data->key;
	        int to   = 10;
	        //int from          = RND_Interval(S, 0, MAXINT / 2);
	        //int to            = RND_Interval(S, 0, MAXINT / 2);

	        // The newly generated message has to be inserted in the local cache
	        lunes_trans_insert(node->data->s_state.blockchain, node->data->latestblock, from, to, transactionid);

	        // Statistics: print in the trace file all the necessary information
	        //		a new message has been generated
	        #ifdef TRACE_DISSEMINATION
	        fprintf(fp_print_trace, "G %010u\n", value);
	        #endif
	        //		obviously the generating node has "seen" (received)
	        //		the locally generated message
	        #ifdef TRACE_DISSEMINATION
	        fprintf(fp_print_trace, "R %010u %010u %010u\n", node->data->key, value, 0);
	        #endif

	        #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
	        // Updating (or initializing) the number of my neighbors
	        node->data->num_neighbors = g_hash_table_size(node->data->state);
	        #endif

	        // Broadcasting the new message to all neighbors
	        lunes_send_trans_to_neighbors(node, transactionid, from, to);
	        // clock - nodeid - txid - from - to - blockid
	        fprintf(stdout, "TS: %.0f,%d,%d,%d,%d,%d\n",
	                simclock,
	                node->data->key,
	                transactionid,
	                from,
	                to,
	                node->data->latestblock);
	    }
	    #endif
	}
}

/****************************************************************************
 *! \brief LUNES_REGISTER: a new SE (in this LP) has been created, LUNES needs to
 *         initialize some data structures
 */
void lunes_user_register_event_handler(hash_node_t *node) {
    double freerider;           // Tmp variabile
    float  threshold;           // Tmp, probabilistic evaluation

    // Only a given percentage of nodes generates new messages
    threshold = RND_Interval(S, (double)0, (double)100);

    node->data->s_state.time_of_next_check = simclock + (RND_Exponential(S, 1) * MEAN_NEW_MESSAGE);
    if (threshold <= PERC_GENERATORS) {
        // Initialization of the time for the generation of new messages
        node->data->s_state.time_of_next_trans = simclock + (RND_Exponential(S, 1) * MEAN_NEW_MESSAGE);
    }else {
        node->data->s_state.time_of_next_trans = -1; // This node will not generate messages
    }
    // Is this node a free-rider or not?
    if (env_freerider_prob > 0) {
        freerider = RND_Interval(S, (double)0, (double)100);
        if (freerider <= env_freerider_prob) {
            node->data->s_state.freerider = 1; // free-rider
        }else {
            node->data->s_state.freerider = 0; // NOT a free-rider
        }
    }
}

/****************************************************************************
 *! \breif LUNES_TRANS: what happens in LUNES when a node receives a TRANS message?
 *
 * @param[in] node: The node receiving the TransMsg
 * @param[in] msg: The TransMsg
 */
void lunes_user_trans_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    #ifdef TXDEBUG
    // Time-To-Live check
    if (msg->trans.trans_static.ttl > 0) {
        // The TTL is still OK
        // Is this node a free-rider?
        if (node->data->s_state.freerider == 0) {
            // Verify the transaction is not known
            if (node->data->key != msg->trans.trans_static.creator &&
                lunes_trans_is_known(node->data->s_state.blockchain, node->data->latestblock, msg->trans.trans_static.transid) == 0) {
                // It has not been received
                lunes_trans_insert(node->data->s_state.blockchain,
                                   node->data->latestblock,
                                   2,
                                   1,
                                   msg->trans.trans_static.transid);

                // clock - nodeid - originalcreartor - timestamp - txid - from - to - blockid
                fprintf(stdout, "TR: %.0f,%d,%d,%.0f,%d,%d,%d,%d\n",
                        simclock,
                        node->data->key,
                        msg->trans.trans_static.creator,
                        msg->trans.trans_static.timestamp,
                        msg->trans.trans_static.transid,
                        2,
                        1,
                        node->data->latestblock);


                #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
                // Updating (or initializing) the number of my neighbors
                gpointer neighbor;
                node->data->num_neighbors = g_hash_table_size(node->data->state);

                // Updating the number of neighbors of forwarder's neighbors
                neighbor = g_hash_table_lookup(node->data->state, &forwarder);
                ((value_element *)neighbor)->num_neighbors = msg->trans.trans_static.num_neighbors;
                #endif

                // Dissemination (to some of) the neighbors
                // NOTE: the TTL is being decremented here!
                lunes_forward_to_neighbors(node,
                                           msg,
                                           --(msg->trans.trans_static.ttl),
                                           msg->trans.trans_static.timestamp,
                                           msg->trans.trans_static.creator,
                                           forwarder);
            }else {
                // The message is already in the block
                #ifdef STALETXDEBUG
                // clock - nodeid - originalcreartor - timestamp - txid, from, to
                fprintf(stdout, "TRS: %.0f,%d,%d,%.0f,%d,%d,%d\n",
                        simclock, node->data->key,
                        msg->block.block_static.creator,
                        msg->block.block_static.timestamp,
                        msg->trans.trans_static.transid,
                        2, 1);
                fflush(stdout);
                #endif
            }
        }else {
            // This node is a free-rider (i.e. no cache management, no forwarding)
            #ifdef FREERIDINGDEBUG
            fprintf(stdout, "%12.2f node: [%5d] message [%5d] dropped for freeriding\n", simclock, node->data->key, msg->trans.trans_static.transid);
            #endif
        }
    }
    #endif
}

/****************************************************************************
 *! \breif LUNES_BLOCK: what happens in LUNES when a node receives a BLOCK message?
 *
 * @param[in] node: The node receiving the BlockMsg
 * @param[in] msg: The BlockMsg
 */
void lunes_user_block_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    // Time-To-Live check
    if (msg->block.block_static.ttl > 0) {
        // The TTL is still OK
        // Is this node a free-rider?
        if (node->data->s_state.freerider == 0) {
            int ltblock         = node->data->latestblock;
            int receivedblockid = msg->block.block_static.minedblock->id;
            // If the attacker receive a block present in his private blockchain empty the buffer
            // and sent all blocks
            if (node->data->attackerid == node->data->key && selfish[2] >= 0) {
                // "Data structure" to save, attacaker, blockchain and attack status
                // selfish[0] = latest mined block in attacker's private blockchain
                // selfish[1] = latest received block from the network for the attacker
                // selfish[2] = status of the attack (-1 = disabled, 0 active and even with the blockchain, 1
                //              attacker is 1 block ahead, 3 attacker sent the mined block
            	#ifdef FORKING
            	int pos = msg->block.block_static.minedblock->position;
            	int selfish_0_ind = getIndexById(&node->data->s_state.blockchain[0], node->data->latestblock, selfish[0]); 
            	int selfish_1_ind = getIndexById(&node->data->s_state.blockchain[0], node->data->latestblock, selfish[1]);
            	int selfish_0_pos = node->data->s_state.blockchain[selfish_0_ind].position;         //position of the latest mined block in attacker's private blockchain
            	int selfish_1_pos = node->data->s_state.blockchain[selfish_1_ind].position;         //position of the latest received block from the network for the attacker
                selfish[1] = pos > selfish_1_pos || selfish[1] <= 0 ? receivedblockid : selfish[1]; //update selfish[1] if the position of the received block is greater
                selfish_1_pos = pos > selfish_1_pos || selfish[1] <= 0 ? pos : selfish_1_pos;       //to determine which is now the block with greatest position received from the network
                int diff_prev =  selfish_0_pos - selfish_1_pos;                                     //advantage with respect to the network
                if (diff_prev == 0) {                                                               //zero blocks in advantage
                    selfish[2] = 0;                                                                 
                } else if (diff_prev == 1) {                                                        //just one block in advantage, risk of spoiling mining effort, propagate the block
                	int previousId = node->data->s_state.blockchain[selfish_0_ind].prevId;          
                	int previousIndex = getIndexById (&node->data->s_state.blockchain[0], node->data->latestblock, previousId);
                	lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[previousIndex]);
                	fprintf(stdout, "BSS: %.0f,%d,%d,%d,%f\n", simclock, node->data->key, node->data->s_state.blockchain[selfish_0_ind].prevId, node->data->s_state.blockchain[selfish_0_ind].position - 1, node->data->hashrate);
                	lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[selfish_0_ind]);
                	fprintf(stdout, "BSS: %.0f,%d,%d,%d,%f\n", simclock, node->data->key, node->data->s_state.blockchain[selfish_0_ind].id, node->data->s_state.blockchain[selfish_0_ind].position, node->data->hashrate);                   
                    selfish[2] = 0;
                }
            	#else
                int diff_prev = selfish[0] - receivedblockid;
                selfish[1] = receivedblockid;
                if (diff_prev == 0) {
                    selfish[0] = receivedblockid;
                    selfish[2] = 0;
                }else if (diff_prev == 1) {
                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[selfish[0]]);
                    // clock - nodeid - blockminedid - hashrate
                    fprintf(stdout, "BSS: %.0f,%d,%d,%f\n", simclock, node->data->key, selfish[0], node->data->hashrate);
                }else if (diff_prev == 2) {
                    for (int b = selfish[0]; b >= 0; --b) {
                        // Broadcasting the mined old blocks to all neighbors
                        lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[selfish[0] - b]);
                        // clock - nodeid - blockminedid - hashrate
                        fprintf(stdout, "BSS: %.0f,%d,%d,%f\n", simclock, node->data->key, selfish[0] - b, node->data->hashrate);
                    }
                    selfish[2] = 0;
                }else if (selfish[0] >= 0) {
                    int first_unp_block = selfish[0] - selfish[2];
                    lunes_send_block_to_neighbors(node, &node->data->s_state.blockchain[first_unp_block]);
                    // clock - nodeid - blockminedid - hashrate
                    fprintf(stdout, "BSS: %.0f,%d,%d,%f\n", simclock, node->data->key, first_unp_block, node->data->hashrate);
                }
                #endif
            }

            #ifdef FORKING
          //  if (node->data->key !=40 || simclock < 200 || simclock > 400){  //to test if a node, (node with id =40 in this case) can easily 
         //                                                    recover previous block is disconnected from the network for 200 time steps
            Block * receivedBlock = msg->block.block_static.minedblock;
            if (is_in_blockchain(&node->data->s_state.blockchain[0], receivedBlock->id, ltblock) == -1){  //if it's a new block
	            Block ** heads = node->data->s_state.heads;
	            int prevId =  receivedBlock->prevId;
	            if (receivedBlock->position == 1){                                         //if it's the first block in the chain
                    add_heads(node->data->s_state.heads, receivedBlock);        	       //added to head-blocks
	            } else {
	            	int pos = is_in_heads (heads, prevId);                                 //!= 1 means the block of given id is among head-blocks
	            	if (pos != -1){                                                        //if previous block is a head block
	            		replace_heads (heads, pos, receivedBlock);                         // new block replaces his previous among head-blocks
	            	} else {                                                               //if the previous block not a head block           	    
	            	    if (is_in_blockchain(&node->data->s_state.blockchain[0], prevId, ltblock)!= -1)	{   //if previous block is not a head block but it's in the blockchain
	            		    int next = is_next_in_blockchain(&node->data->s_state.blockchain[0], node->data->latestblock, receivedBlock->id);  //Try to find the following of the received block
	            		    while (next != -1){                                            //try to find, if any, the last descendant
                                next = is_next_in_blockchain(&node->data->s_state.blockchain[0], node->data->latestblock, node->data->s_state.blockchain[next].id);
	            		    }
	            		    if (next != -1){                                                //if the next block exists put the lastest of the chain as a head-block
	            		    	add_heads (heads, &node->data->s_state.blockchain[next]);
	            		    } else {                                                        // if the next block doesn't exist put the block as a head-block
	            		    	add_heads (heads, receivedBlock);

	            		    }
	            	    } else {                                                            // previous node not in blockchain, received block is an orphan block
	            	                                                                      	//ask for the minor position between the position of the received block 
                                                                                            //and the position of the longest head-block
                            int positionToAsk = 1;
                            if (heads_greater_position(heads) != -1){                       
                                positionToAsk = heads[heads_greater_position(heads)]->position + 1; //position of the longest head-block
                            }
	            	    	if (receivedBlock->position - 1 < positionToAsk ){   
	                            positionToAsk = receivedBlock->position == 1 ? 1 : receivedBlock->position -1;  //you will ask for all blocks with position >=positionToAsk
	            	    	}
			        	    GHashTableIter iter;
			                gpointer       key, destination;
			                hash_node_t *  sender, *receiver; // Sender and receiver nodes in the global hashtable

			                g_hash_table_iter_init(&iter, node->data->state);
			                while (g_hash_table_iter_next(&iter, &key, &destination)) {
			                	float threshold = RND_Interval(S, (double)0, (double)100);
			                    sender   = hash_lookup(stable, node->data->key);             // This node
			                    receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor
			                    if (threshold > 50){         // not sending the ask message to all the neighbors, in order to avoid useless messages
				                    execute_ask_block(simclock + FLIGHT_TIME,
				                                      sender,
				                                      receiver,
				                                      env_max_ttl,
				                                      positionToAsk,
				                                      simclock + FLIGHT_TIME,
				                                      node->data->key);

					                #ifdef ASKBLOCKDEBUG
					                // clock - nodeid - blockid
					                fprintf(stdout, "ABS: %.0f,%d,%d\n", simclock, node->data->key, positionToAsk);
					                #endif
			                    }
			                }
	            	    }
	            	}
	            }
	            node->data->s_state.blockchain[ltblock] =  *msg->block.block_static.minedblock;
	            node->data->latestblock += 1;

	            fprintf(stdout, "BR: %.0f,%d,%d,%.0f,%d,%d\n",
		                simclock, 
		                node->data->key,
		                msg->block.block_static.creator,
		                msg->block.block_static.timestamp,
		                receivedblockid,
		                msg->block.block_static.minedblock->position);

	            lunes_forward_to_neighbors(node,
	                                       msg,
	                                       --(msg->block.block_static.ttl),
	                                       msg->block.block_static.timestamp,
	                                       msg->block.block_static.creator,
	                                       forwarder);
            }//}
            #else
            // If the received block is the next one in the local blockchain add it
            if (ltblock == receivedblockid) {  
                // clock - nodeid - originalcreartor - timestamp - blockid
 
                fprintf(stdout, "BR: %.0f,%d,%d,%.0f,%d,%d\n",
                        simclock,
                        node->data->key,
                        msg->block.block_static.creator,
                        msg->block.block_static.timestamp,
                        receivedblockid);
                node->data->s_state.blockchain[ltblock].id = ltblock;

                // It has not been received, increment latestblock and add all transactions
                node->data->internal_timer = 0;
           
                // (add the block to the blockchain)
                node->data->latestblock += 1;
                // store only the pointer to reduce RAM usage
                node->data->s_state.blockchain[ltblock] = *msg->block.block_static.minedblock;
                #ifdef TXDEBUG
                //for (int b = 0; b < msg->block.block_static.minedblock->latesttrans; ++b) {
                //    node->data->s_state.blockchain[ltblock].trans[b].id   = msg->block.block_static.minedblock->trans[b].id;
                //    node->data->s_state.blockchain[ltblock].trans[b].from = msg->block.block_static.minedblock->trans[b].from;
                //    node->data->s_state.blockchain[ltblock].trans[b].to   = msg->block.block_static.minedblock->trans[b].to;
                //}
                #endif

                // Dissemination (to some of) the neighbors
                // NOTE: the TTL is being decremented here!
                lunes_forward_to_neighbors(node,
                                           msg,
                                           --(msg->block.block_static.ttl),
                                           msg->block.block_static.timestamp,
                                           msg->block.block_static.creator,
                                           forwarder);
            }else {
                // If the node's blockchain is not updated ask the neighbors for the current blocks
                // If the node is mining the ltblock and the message is for ltblock + 1 we need to
                // ask for the ltblock
                if (msg->block.block_static.minedblock->id > node->data->latestblock) {
                    GHashTableIter iter;
                    gpointer       key, destination;
                    hash_node_t *  sender, *receiver; // Sender and receiver nodes in the global hashtable

                    g_hash_table_iter_init(&iter, node->data->state);
                    while (g_hash_table_iter_next(&iter, &key, &destination)) {
                        sender   = hash_lookup(stable, node->data->key);             // This node
                        receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor
                        execute_ask_block(simclock + FLIGHT_TIME,
                                          sender,
                                          receiver,
                                          env_max_ttl,
                                          node->data->latestblock,
                                          simclock + FLIGHT_TIME,
                                          node->data->key);
                    }

                    #ifdef ASKBLOCKDEBUG
                    // clock - nodeid - blockid
                    fprintf(stdout, "ABS: %.0f,%d,%d\n", simclock, node->data->key, node->data->latestblock);
                    #endif
                }

                #ifdef STALEBLOCKDEBUG
                // Block is stale
                // clock - nodeid - originalcreartor - timestamp - blockid - latestblock
                fprintf(stdout, "BRS: %.0f,%d,%d,%.0f,%d,%d\n",
                        simclock, node->data->key,
                        msg->block.block_static.creator,
                        msg->block.block_static.timestamp,
                        msg->block.block_static.minedblock->id,
                        node->data->latestblock);
                fflush(stdout);
                #endif
                
            }
            #endif
        }else {
            // This node is a free-rider (i.e. no cache management, no forwarding)
            #ifdef FREERIDINGDEBUG
            fprintf(stdout, "%12.2f node: [%5d] message [%5d] dropped for freeriding\n", simclock, node->data->key, msg->block.block_static.minedblock.id);
            #endif
        }
    }
}

/****************************************************************************
 *! \breif LUNES_ASKBLOCK: what happens in LUNES when a node receives a ASKBLOCK message?
 *
 * @param[in] node: The node receiving the AskMsg
 * @param[in] msg: The AskMsg
 */
void lunes_user_askblock_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    // Time-To-Live check
    if (msg->block.block_static.ttl > 0) {
        // The TTL is still OK
        // If the received block is in the local blockchain send it
      
        #ifdef FORKING
        int still=1;                                                        //boolean. 1 if there are still blocks to ask for
        int from =0;                                                        //index where starting to search for a block of a given position
        while (still == 1){                                                 
	        tTuple tuple = find_block_given_position (&node->data->s_state.blockchain[0], from, node->data->latestblock, msg->askblock.askblock_static.blockid );  //msg.blockid actually stays for position
	        Block * b = tuple.block;
	        if (b != NULL){
                from = tuple.from + 1;                                       //blocks with smaller indexes ignored
	            lunes_responde_ask_block(node, b, msg->askblock.askblock_static.creator);
	        } else {                                                         // no more blocks to ask
              still = 0;
	        }
	    }
        #else

        if (node->data->latestblock > msg->askblock.askblock_static.blockid && node->data->attackerid == node->data->key) {
            lunes_responde_ask_block(node, &node->data->s_state.blockchain[msg->askblock.askblock_static.blockid], msg->askblock.askblock_static.creator);
        }
        #endif
    }
}


#ifdef DOS

void lunes_dos_user_control_handler(hash_node_t *node) {
    //initialization
    if (simclock == 5){ //to do just once
        GHashTableIter iter;
        gpointer       key, destination;
        g_hash_table_iter_init(&iter, node->data->state);
        int count = 0;
            // All neighbors
        while (g_hash_table_iter_next(&iter, &key, &destination)) {
            count = count +1;
        }
        node->data->num_neighbors = count;
    }

    //transaction creation by the victim
    if ((int)simclock % env_max_ttl == 0 && simclock + env_max_ttl < 5000){  //da parametrizzare
        if (node->data->key == victim){
            fprintf(stdout, "victim: %d\n", victim);
            if (env_dissemination_mode == BROADCAST || env_dissemination_mode == GOSSIP_FIXED_PROB ){
                lunes_send_trans_to_neighbors(node, 0,0,0);
            } else {
                node->data->s_state.received = simclock;        
                TransMsg     msg;
                // Defining the message type
                msg.trans_static.type = 'T';
                msg.trans_static.timestamp = simclock;
                msg.trans_static.ttl       = env_max_ttl;
              //  msg.trans_static.transid   = RND_Interval(S, 0, MAXINT - 1);
                msg.trans_static.creator   = node->data->key;
                lunes_forward_to_neighbors(node,
                                               &msg,
                                               --(msg.trans_static.ttl),
                                               msg.trans_static.timestamp,
                                               msg.trans_static.creator,
                                               node->data->key);
            }
            node->data->s_state.received = simclock;
        }
        //else if (actual_dos_nodes > node->data->key){   //attackers are like they already received the message 
        else if (node->data->attackerid == 1){ 
            node->data->s_state.received = -1;
        }
        else { //honest nodes that are not the victim
            node->data->s_state.received = 0;            //message has to be received
        }

        //management of fail-safe machanism
    } else if (env_dissemination_mode == DANDELIONPLUS &&
            node->data->s_state.received > 0 &&                                          //just for nodes that managed the message during the stem phase
            //(node->data->key >= actual_dos_nodes || node->data->key == victim) &&      //attackers excluded
            (node->data->attackerid != 1 || node->data->key == victim) &&                //if it's the victim or a honest node 
            (simclock - node->data->s_state.received > dand_plus_waiting) ){             //message not received back
            TransMsg     msg;
            // Defining the message type
            msg.trans_static.type = 'T';
            msg.trans_static.timestamp = node->data->s_state.received;
            msg.trans_static.ttl       = env_dandelion_fluff_steps -1;               //just go to the fluff phase, broadcast the transaction
          //  msg.trans_static.transid   = RND_Interval(S, 0, MAXINT - 1);
            msg.trans_static.creator   = victim;
            lunes_forward_to_neighbors(node,
                                           &msg,
                                           --(msg.trans_static.ttl),
                                           msg.trans_static.timestamp,
                                           msg.trans_static.creator,
                                           node->data->key);
            //lunes_send_trans_to_neighbors(node, 0,0,0);
            node->data->s_state.received = -1;
    }
}

void lunes_dos_user_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    // Time-To-Live check
    if (msg->trans.trans_static.ttl > 0 ) {
        if (node->data->s_state.received == 0) {                            //still didn't receive anything and it's not an attacker nor the creator
            if (msg->trans.trans_static.ttl >= env_dandelion_fluff_steps){   //if we're in the stem phase
                node->data->s_state.received = simclock;                    //information needed for dandelion++ fail-safe mechanism
            } else {                                                        //we're in the fluff phase, nothing more to be done in this epoch
                node->data->s_state.received = -1;
            }

            fprintf(stdout, "TR: %.0f,%d, %d\n", simclock, node->data->key, msg->trans.trans_static.ttl);
            if ((int)simclock % env_max_ttl != env_max_ttl - 1){            //epoch is not over
                lunes_forward_to_neighbors(node,
                                            msg,
                                            --(msg->trans.trans_static.ttl),
                                            msg->trans.trans_static.timestamp,
                                            msg->trans.trans_static.creator,
                                            node->data->key);
            }
        }
        else if (msg->trans.trans_static.ttl < env_dandelion_fluff_steps){   //received it back during the fluff phase, work done
            node->data->s_state.received = -1;
        }
        //if the creator receives back the transaction during the stem phase, then it still relays it
        else if (node->data->s_state.received > 0 &&                      /* implicitly msg->trans.trans_static.ttl > env_dandelion_fluff_steps*/
                (env_dissemination_mode == DANDELIONPLUS || env_dissemination_mode == DANDELION) &&
                (int)simclock % env_max_ttl != env_max_ttl - 1){
                lunes_forward_to_neighbors(node,
                                           msg,
                                           --(msg->trans.trans_static.ttl),
                                           msg->trans.trans_static.timestamp,
                                           msg->trans.trans_static.creator,
                                           node->data->key);
                node->data->s_state.received = simclock;
        }
    }
}
#endif