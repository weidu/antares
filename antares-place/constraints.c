#include <stdlib.h>
#include <util.h>

#include <anetlist/net.h>

#include "constraints.h"

void constraints_init(struct anetlist *a)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	
	inst = a->head;
	while(inst != NULL) {
		constraint = alloc_type(struct constraint);
		constraint->same_slice = NULL;
		constraint->chain_above = NULL;
		constraint->chain_below = NULL;
		constraint->lock = NULL;
		constraint->current = NULL;
		inst->user = constraint;
		inst = inst->next;
	}
}

void constraints_lock(struct anetlist_instance *inst, struct resmgr_bel *bel)
{
	struct constraint *constraint;
	
	constraint = inst->user;
	constraint->lock = bel;
}

void constraints_infer_rel(struct anetlist *a)
{
}

void constraints_free(struct anetlist *a)
{
	struct anetlist_instance *inst;

	inst = a->head;
	while(inst != NULL) {
		free(inst->user);
		inst = inst->next;
	}
}
