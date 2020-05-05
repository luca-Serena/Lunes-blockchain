/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	See "utils.c" description
 *              -	Function prototypes
 *              -	Some constants and macros
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __UTILS_H
#define __UTILS_H

#include <glib.h>
#include "sim-parameters.h"
#include "entity_definition.h"

#define UNUSED    __attribute__ ((__unused__))
#define ASSERT(_cond, _act)     { if (!(_cond)) { printf _act; printf("\n"); assert(_cond); } }

#define TIMER_NOW(_t)           gettimeofday(&_t, NULL)
#define TIMER_SECONDS(_t)       ((double)(_t).tv_sec + (_t).tv_usec * 1e-6)
#define TIMER_DIFF(_t1, _t2)    (TIMER_SECONDS(_t1) - TIMER_SECONDS(_t2))

#define YES        '1'
#define NO         '0'
#define HASHPOW    pow(2, 32)

/*---------------------------------------------------------------------------*/
//	The Simulated Entity can be migrated or not?
#define MIGRABLE        '1'
#define NOT_MIGRABLE    '0'

/* ************************************************************************ */
/*                      Hash Tables		                                    */
/* ************************************************************************ */
enum HASH_TYPE {
    GSE, /* Global hash table of simulated entities */
    LSE, /* Hash table of locally simulated entities */
};

typedef struct hash_node_t {
    struct hash_data_t *data;
    struct hash_node_t *next;
} hash_node_t;

typedef struct hash_t {
    struct hash_node_t **bucket;
    int                  count;
    int                  size;
} hash_t;

/* ************************************************************************ */
/*                      Lists		                                        */
/* ************************************************************************ */
typedef struct se_list_n {
    struct hash_node_t *node;
    struct se_list_n *  next;
} se_list_n;

typedef struct se_list {
    struct se_list_n *head;
    struct se_list_n *tail;
    int               size;
} se_list;

/* ************************************************************************ */
/*                      Prototypes		                                    */
/* ************************************************************************ */
int  hash(hash_t *, int);
void hash_init(hash_t *, int);
int  hash_delete(enum HASH_TYPE, hash_t *, int);
void list_init(se_list *);
void list_add(se_list *, hash_node_t *);
hash_node_t *hash_lookup(hash_t *, int);
hash_node_t *hash_insert(enum HASH_TYPE, hash_t *, struct hash_data_t *, int, int);

struct hash_node_t *list_del(se_list *);

#endif /* __UTILS_H */
