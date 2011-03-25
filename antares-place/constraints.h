#ifndef __CONSTRAINTS_H
#define __CONSTRAINTS_H

#include <anetlist/net.h>

#include "resmgr.h"

struct constraint_group {
	struct anetlist_instance *inst;
	struct constraint_group *next;
};

struct constraint {
	struct constraint_group *group;			/* < instances must be placed in the same slice */
	struct anetlist_instance *chain_above, *chain_below;	/* < instances must be placed in a column */
	struct resmgr_site *lock;				/* < instance is locked to this BELs */
	int lock_bel_index;
	struct resmgr_site *current;				/* < current placement, NULL if unplaced */
	int current_bel_index;
};

void constraints_init(struct anetlist *a);
void constraints_same_slice(struct anetlist_instance *i1, struct anetlist_instance *i2);
void constraints_lock(struct anetlist_instance *inst, struct resmgr_site *s, int bel_index);
void constraints_infer_rel(struct anetlist *a);
void constraints_free(struct anetlist *a);

#endif /* __CONSTRAINTS_H */
