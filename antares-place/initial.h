#ifndef __INITIAL_H
#define __INITIAL_H

#include "resmgr.h"
#include "rtree.h"

void *rtrees_random_pick(struct rtree_node **roots, int n_roots, unsigned int rnd);

void initial_placement(struct resmgr *r);

#endif /* __INITIAL_H */
