#ifndef __CONSTRAINTS_H
#define __CONSTRAINTS_H

#include <anetlist/net.h>

#include "resmgr.h"

struct constraint {
	struct anetlist_instance *same_slice;			/* < instance must be in the same slice as this one */
	struct anetlist_instance *chain_above, *chain_below;	/* < instances must be placed in a column */
	struct resmgr_bel *lock;				/* < instance is locked to this BELs */
	struct resmgr_bel *current;				/* < current placement, NULL if unplaced */
};

void constraints_init(struct anetlist *a);
void constraints_lock(struct anetlist_instance *inst, struct resmgr_bel *bel);
void constraints_infer_rel(struct anetlist *a);
void constraints_free(struct anetlist *a);

#endif /* __CONSTRAINTS_H */
