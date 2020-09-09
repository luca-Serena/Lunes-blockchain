/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file are defined the message types and their structures
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __MESSAGE_DEFINITION_H
#define __MESSAGE_DEFINITION_H

#include "entity_definition.h"

/*---- M E S S A G E S    D E F I N I T I O N ---------------------------------*/

// Model messages definition
typedef struct _trans_msg      TransMsg; // Interactions among messages
typedef struct _link_msg       LinkMsg;  // Network constructions
typedef struct _migr_msg       MigrMsg;  // Migration message
typedef struct _block_msg      BlockMsg; // Block mined
typedef struct _askblock_msg   AskMsg;   // Ask for a block (update the blockchain)
typedef union   msg            Msg;

// General note:
//	each type of message is composed of a static and a dynamic part
//	-	the static part contains a pre-defined set of variables, and the size
//		of the dynamic part (as number of records)
//	-	a dynamic part that is composed of a sequence of records

// **********************************************
// TRANSACTION MESSAGES
// **********************************************
//
/*! \brief Static part of transaction messages */
struct _trans_static_part {
    char           type;                    // Message type
    float          timestamp;               // Timestep of creation (of the message)
    unsigned short ttl;                     // Time-To-Live
    int            transid;                 // Message Identifier
    unsigned int   creator;                 // ID of the original sender of the message
   // #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
    unsigned int   num_neighbors;           // Number of neighbors of forwarder
    //#endif
};
//
/*! \brief Transaction message */
struct _trans_msg {
    struct  _trans_static_part trans_static;
};

// **********************************************
// LINK MESSAGES
// **********************************************
/*! \brief Record definition for dynamic part of link messages */
struct _link_record {
    unsigned int key;
    unsigned int value;
};
//
/*! \brief Static part of link messages */
struct _link_static_part {
    char type; // Message type
};
//
/*! \brief Link message */
struct _link_msg {
    struct  _link_static_part link_static; // Static part
};

// **********************************************
// MIGRATION MESSAGES
// **********************************************
//
/*! \brief Static part of migration messages */
struct _migration_static_part {
    char          type;        // Message type
    static_data_t s_state;     // Static part of the SE state: it is the same
                               //	 of the static part of the simulated entities state
    unsigned int  dyn_records; // Number of records in the dynamic part of the message
};
//
/*! \brief Dynamic part of migration messages */
struct _migration_dynamic_part {
    struct state_element records[MAX_MIGRATION_DYNAMIC_RECORDS]; // It is an array of records
};
//
/*! \brief Migration message */
struct _migr_msg {
    struct  _migration_static_part  migration_static;  // Static part
    struct  _migration_dynamic_part migration_dynamic; // Dynamic part
};

// **********************************************
// BLOCK PROPAGATION MESSAGES
// **********************************************
//
/*! \brief Static part of block mined messages */
struct _block_static_part {
    char           type;                       // Message type
    float          timestamp;                  // Timestep of creation (of the message)
    unsigned short ttl;                        // Time-To-Live
    Block *        minedblock;                 // Block mined
    unsigned int   creator;                    // ID of the original sender of the message
};
//
/*! \brief Block message */
struct _block_msg {
    struct  _block_static_part block_static;   // Static part
};

// **********************************************
// BLOCK REQUEST MESSAGES
// **********************************************
//
/*! \brief Static part of block mined messages */
struct _askblock_static_part {
    char           type;          // Message type
    float          timestamp;     // Timestep of creation (of the message)
    unsigned short ttl;           // Time-To-Live
    int            blockid;       // Block mined
    unsigned int   creator;       // ID of the original sender of the message
};
//
/*! \brief Block message */
struct _askblock_msg {
    struct  _askblock_static_part askblock_static;   // Static part
};


/*! \brief Union structure for all types of messages */
union msg {
    char     type;
    LinkMsg  link;
    TransMsg trans;
    MigrMsg  migr;
    BlockMsg block;
    AskMsg   askblock;
};
/*---------------------------------------------------------------------------*/

#endif /* __MESSAGE_DEFINITION_H */
