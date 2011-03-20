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
		constraint->ctype = CONSTRAINT_NONE;
		constraint->cobj = NULL;
		constraint->current = NULL;
		inst->user = constraint;
		inst = inst->next;
	}
}

void constraints_lock(struct anetlist_instance *inst, struct resmgr_bel *bel)
{
	struct constraint *constraint;
	
	constraint = inst->user;
	constraint->ctype = CONSTRAINT_LOCKED;
	constraint->cobj = bel;
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
