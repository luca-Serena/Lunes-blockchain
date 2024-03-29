/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file is defined the state of the simulated entitiesFORKI
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __ENTITY_DEFINITION_H
#define __ENTITY_DEFINITION_H

#include "lunes_constants.h"


/*---- E N T I T I E S    D E F I N I T I O N ---------------------------------*/

/*! \brief Structure of "value" in the hash table of each node
 *         in LUNES used to implement neighbors and its properties
 */
typedef struct v_e {
    unsigned int value;                     // Value
    #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
    unsigned int num_neighbors;             // Number of neighbors of each neighbor of a given node
    #endif
} value_element;

/*! \brief Records composing the local state (dynamic part) of each SE
 *         NOTE: no duplicated keys are allowed
 */
struct state_element {
    unsigned int  key;      // Key
    value_element elements; // Value
};

/*! \brief Structure of a transaction in each block */
typedef struct transaction_element {
    int id;   // ID of the transaction
} Transaction;

/*! \brief Structure of a block in each node */
typedef struct blockchain_element {
    int         id;          // ID of the block
    int         latesttrans; // ID of the lastest transacation (used as index)
    int         prevId;      // ID of the previoud block in the chain
    int         position;    // position of the block in the blockchain
    #ifdef TXDEBUG
    Transaction trans[500];  // Mean # of transactions per block (less is better for performance)
    #endif
} Block;

/*! \brief Static part of the SE state */
typedef struct static_data_t {
    Block   *  heads[10];                     // head blocks of the forks                          
    char      changed;                        // ON if there has been a state change in the last timestep
    char      freerider;                      // 1 if free-rider, 0 not free-rider
    float     time_of_next_trans;             // Timestep in which the next new transaction will be created and sent
    float     time_of_next_check;             // Timestep in which the next check for new block will be created and sent
    Block     blockchain[3000];               // Array of blocks
    #ifdef DOS
    short     received;                       //0 not received anything, !=0 received the message, -1 received the message back in the fluff phase
    #endif
} static_data_t;

/*! \brief SE state definition */
typedef struct hash_data_t {
    int           key;                    // SE identifier
    int           lp;                     // Logical Process ID (that is the SE container)
    double        hashrate;               // Hashrate of the node (if a miner)
    int           attackerid;             // ID of the attacker (equal to key)
    int           miner;                  // Boolean: is the node a miner?
    int           internal_timer;         // Used to track mining activity
    int           latestblock;            // ID of the latest block (used as index)
    static_data_t s_state;                // Static part of the SE local state
    GHashTable *  state;                  // Local state as an hash table (glib) (dynamic part)
  //  #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
    unsigned int  num_neighbors;          // Number of SE's neighbors (dynamically updated)
    //#endif
} hash_data_t;

#endif /* __ENTITY_DEFINITION_H */
