/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	Utility functions, in particular data structures management
 *                      (e.g. hash table, list)
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
#include "utils.h"

extern TSeed   Seed, *S;
extern int     NSIMULATE;
extern double *rates;
extern int     atk_hashrate;
extern int     number_dos_nodes;
extern int     env_miners_count;
extern int    *attackers;

/* ************************************************************************* */
/*           D A T A    S T R U C T U R E S    M A N A G E M E N T           */
/* ************************************************************************* */

/*! \brief Custom hash generation function */
int hash(hash_t *tptr, int x) {
    return(x % tptr->size);
}

/*! \brief Hash table initialization
 */
void hash_init(hash_t *tptr, int size) {
    tptr->size   = size;
    tptr->count  = 0;
    tptr->bucket = (hash_node_t **)calloc(tptr->size, sizeof(hash_node_t *));
    return;
}

/*! \brief Lookup of a simulated entity (hash table)
 */
hash_node_t *hash_lookup(hash_t *tptr, int key) {
    int          h;
    hash_node_t *node;

    h = hash(tptr, key);
    for (node = tptr->bucket[h]; node != NULL; node = node->next) {
        if (node->data->key == key) {
            break;
        }
    }

    return(node);
}

/*! \brief Insertion of a new simulated entity (hash table)
 */
hash_node_t *hash_insert(enum HASH_TYPE type, hash_t *tptr, struct hash_data_t *data, int key, int lp) {
    hash_node_t *node, *tmp;
    int          h;
    // Make global
    int attacker = 37;
    //int victim   = 37;

    if ((tmp = hash_lookup(tptr, key))) {
        return(tmp);
    }

    h    = hash(tptr, key);
    node = (struct hash_node_t *)malloc(sizeof(hash_node_t));
    ASSERT((node != NULL), ("hash_insert: malloc error"));

    // Inserting the SE in the global hashtable
    if (type == GSE) {
        node->data = (struct hash_data_t *)malloc(sizeof(hash_data_t));
        ASSERT((node->data != NULL), ("hash_insert: malloc error"));
        node->data->key = key;
    } // Inserting the SE in the local hashtable
    else if (type == LSE) {
        node->data = data;
    }
    node->data->lp = lp;

    // Init some values
    node->data->internal_timer = 0;
    node->data->latestblock    = 0;
    node->data->attackerid     = -1;
    node->data->miner          = 0;
    node->data->hashrate       = 0;
    node->data->s_state.blockchain[0].latesttrans = 0;
    node->data->s_state.blockchain[0].id          = 0;

    // Enabled only during a 51% test
    if (atk_hashrate > 0 && tptr->count == attacker) {
        node->data->miner      = 1;
        node->data->attackerid = tptr->count;
        node->data->hashrate   = atk_hashrate;
    } else {/*
        // Enabled only during a DOS test (filtering mode)
        // Target of the attack is the node 337
        if (tptr->count < number_dos_nodes && tptr->count != victim) {
            node->data->attackerid = tptr->count;
        }*/

        //Get and save the hashrate for this node
        if (tptr->count < env_miners_count) {
            node->data->hashrate = rates[tptr->count];
            node->data->miner    = 1;
        } else {
            node->data->miner = 0;
        }
    }
    #ifdef DOS
    if (number_dos_nodes > 0){					//setting which nodes are malicious
        for (int i = 0; i < number_dos_nodes; i++){
            if (node->data->key == attackers[i]){
                node->data->attackerid = 1;
                break;
            }
        }
    }
    #endif



    node->next      = tptr->bucket[h];
    tptr->bucket[h] = node;
    tptr->count    += 1;

    return(node);
}

/*! \brief Deletion of a node (hash table)
 */
int UNUSED hash_delete(enum HASH_TYPE type, hash_t *tptr, int key) {
    hash_node_t *node, *last;
    int          h;

    h = hash(tptr, key);
    for (node = tptr->bucket[h]; node; node = node->next) {
        if (node->data->key == key) {
            break;
        }
    }

    if (node == NULL) {
        return(-1);
    }

    if (node == tptr->bucket[h]) {
        tptr->bucket[h] = node->next;
    }else {
        for (last = tptr->bucket[h]; last && last->next; last = last->next) {
            if (last->next == node) {
                break;
            }
        }
        last->next = node->next;
    }

    tptr->count -= 1;

    if (type == GSE) {
        free(node->data);
    }

    free(node);
    return(1);
}

/*---------------------------------------------------------------------------*/

/*! \brief List initialization
 */
void list_init(se_list *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/*! \brief Insertion of a new node (list)
 */
void list_add(se_list *list, hash_node_t *node) {
    se_list_n *list_n;

    list_n = (struct se_list_n *)malloc(sizeof(se_list_n));
    ASSERT((list_n != NULL), ("list_add: malloc error"));

    list_n->node = node;
    list_n->next = NULL;

    if (list->head == NULL) {
        list->head = list_n;
    }

    if (list->tail != NULL) {
        list->tail->next = list_n;
    }

    list->tail  = list_n;
    list->size += 1;
}

/*! \brief Deletion of a node (list)
 */
struct hash_node_t *list_del(se_list *list) {
    struct se_list_n *  head = NULL;
    struct hash_node_t *node = NULL;

    if (list->size > 0) {
        list->size -= 1;
        head        = list->head;
        node        = list->head->node;
        list->head  = list->head->next;

        if (list->size <= 0) {
            list->tail = NULL;
        }
        free(head);
    }

    return(node);
}

/*---------------------------------------------------------------------------*/
