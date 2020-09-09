/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	Function prototypes
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __LUNES_H
#define __LUNES_H


#include "utils.h"
#include "entity_definition.h"

#ifdef TXDEBUG
void lunes_trans_insert(Block *, int, int, int, int);
int lunes_trans_is_known(Block *, int, int);
#endif
void lunes_real_forward(hash_node_t *, Msg *, unsigned short, float, unsigned int, unsigned int);
void lunes_forward_to_neighbors(hash_node_t *, Msg *, unsigned short, float, unsigned int, unsigned int);
void lunes_send_trans_to_neighbors(hash_node_t *, int, int, int);
void lunes_send_block_to_neighbors(hash_node_t *, Block *);
void lunes_responde_ask_block(hash_node_t *, Block *, int);

// LUNES handlers
void lunes_user_trans_event_handler(hash_node_t *, int, Msg *);
void lunes_user_block_event_handler(hash_node_t *, int, Msg *);
void lunes_user_askblock_event_handler(hash_node_t *, int, Msg *);
void lunes_user_register_event_handler(hash_node_t *);
void lunes_user_control_handler(hash_node_t *);
double mining_probability(double, double);

// Support functions
void lunes_dot_tokenizer(char *, int *, int *);
void lunes_load_graph_topology();
/*int is_in_heads(Block * , int );
void replace_heads(Block *, int , Block * );
int heads_greater_position (Block * );*/
int minimum_pos (Block **);
typedef struct {
    Block * block;
    int from;
} tTuple;
#endif /* __LUNES_H */

#ifdef DOS
void lunes_dos_user_control_handler(hash_node_t *);
void lunes_dos_user_event_handler(hash_node_t *, int, Msg *);
#endif