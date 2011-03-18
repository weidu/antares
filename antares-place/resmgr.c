#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <anetlist/net.h>
#include <anetlist/entities.h>
#include <anetlist/bels.h>
#include <chip/db.h>

#include "resmgr.h"

static int cmp_control_set(struct resmgr_control_set *cs1, struct resmgr_control_set *cs2)
{
	return (
		    (cs1->clock.inst == cs2->clock.inst) && (cs1->clock.output == cs2->clock.output)
		 && (cs1->ce.inst == cs2->ce.inst) && (cs1->ce.output == cs2->ce.output)
		 && (cs1->sr.inst == cs2->sr.inst) && (cs1->sr.output == cs2->sr.output)
		);
} 

static void add_control_set(struct resmgr *r, struct resmgr_control_set *cs)
{
	int i;
	
	for(i=0;i<r->n_control_sets;i++)
		if(cmp_control_set(&r->control_sets[i], cs))
			return;
	
	r->n_control_sets++;
	r->control_sets = realloc(r->control_sets, r->n_control_sets*sizeof(struct resmgr_control_set));
	if(r->control_sets == NULL) abort();
	r->control_sets[r->n_control_sets-1] = *cs;
}

static void populate_control_sets(struct resmgr *r, struct anetlist *a)
{
	struct anetlist_instance *inst;
	struct resmgr_control_set cs;
	
	inst = a->head;
	while(inst != NULL) {
		if(inst->e == &anetlist_bels[ANETLIST_BEL_FDRE]) {
			if(inst->inputs[ANETLIST_BEL_FDRE_C] != NULL) {
				cs.clock.inst = inst->inputs[ANETLIST_BEL_FDRE_C]->inst;
				cs.clock.output = inst->inputs[ANETLIST_BEL_FDRE_C]->pin;
			} else {
				fprintf(stderr, "Flip flop %s has no connection on C pin. Unclocked flip-flops are not supported\n", inst->uid);
				exit(EXIT_FAILURE);
			}
			if(inst->inputs[ANETLIST_BEL_FDRE_CE] != NULL) {
				cs.ce.inst = inst->inputs[ANETLIST_BEL_FDRE_CE]->inst;
				cs.ce.output = inst->inputs[ANETLIST_BEL_FDRE_CE]->pin;
			} else {
				cs.ce.inst = NULL;
				cs.ce.output = 0;
			}
			if(inst->inputs[ANETLIST_BEL_FDRE_R] != NULL) {
				cs.sr.inst = inst->inputs[ANETLIST_BEL_FDRE_R]->inst;
				cs.sr.output = inst->inputs[ANETLIST_BEL_FDRE_R]->pin;
			} else {
				cs.sr.inst = NULL;
				cs.sr.output = 0;
			}
			add_control_set(r, &cs);
		}
		inst = inst->next;
	}
}

struct resmgr *resmgr_new(struct anetlist *a, struct db *db)
{
	struct resmgr *r;
	
	r = alloc_type(struct resmgr);
	
	r->n_control_sets = 0;
	r->control_sets = NULL;
	populate_control_sets(r, a);
	
	printf("Number of unique control sets: %d\n", r->n_control_sets);
	
	return r;
}

void resmgr_free(struct resmgr *r)
{
	free(r->control_sets);
	free(r);
}
