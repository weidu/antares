#include <stdlib.h>

#include <anetlist/net.h>
#include <anetlist/entities.h>
#include <anetlist/bels.h>

#include <mtwist/mtwist.h>

#include "resmgr.h"
#include "rtree.h"
#include "constraints.h"
#include "initial.h"

void *rtrees_random_pick(struct rtree_node **roots, int n_roots, unsigned int rnd)
{
	int total_count;
	int i;
	
	total_count = 0;
	for(i=0;i<n_roots;i++)
		total_count += roots[i]->count;
	rnd %= total_count;
	i = 0;
	while(rnd >= roots[i]->count) {
		i++;
		rnd -= roots[i]->count;
	}
	return rtree_get(roots[i], rnd);
}

static int get_bel_index(struct anetlist_entity *e)
{
	int len;
	int index;
	
	len = sizeof(anetlist_bels)/sizeof(anetlist_bels[0]);
	index = e - anetlist_bels;
	if(index >= len)
		index = -1;
	return index;
}

static void place_locked(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;

	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL) && (constraint->lock != NULL))
			resmgr_place(r, inst, constraint->lock);
		inst = inst->next;
	}
}

static void place_chains(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	
	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL) 
			&& ((constraint->chain_below != NULL) || (constraint->chain_above == NULL))) {
			/* TODO */
		}
		inst = inst->next;
	}
}

static void place_same_slice(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	
	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL) && (constraint->same_slice != NULL)) {
			/* TODO */
		}
		inst = inst->next;
	}
}

static void place_unconstrained(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	int bel;
	struct rtree_node *roots[2];
	
	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL)
		  && (constraint->same_slice == NULL)
		  && (constraint->chain_above == NULL)
		  && (constraint->chain_below == NULL)
		  && (constraint->lock == NULL)
		  && (constraint->current == NULL)) {
			bel = get_bel_index(inst->e);
			switch(bel) {
				case ANETLIST_BEL_LUT6_2:
					resmgr_place(r, inst, rtrees_random_pick(&r->free_lut, 1, mts_lrand(r->prng)));
					break;
				case ANETLIST_BEL_IOBM:
					roots[0] = r->free_iobm;
					roots[1] = r->free_iobs;
					resmgr_place(r, inst, rtrees_random_pick(roots, 2, mts_lrand(r->prng)));
					break;
				default:
					fprintf(stderr, "Unsupported BEL: %s\n", inst->e->name);
					exit(EXIT_FAILURE);
			}
		}
		inst = inst->next;
	}
}

void initial_placement(struct resmgr *r)
{
	place_locked(r);
	place_chains(r);
	place_same_slice(r);
	place_unconstrained(r);
}
