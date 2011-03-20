#ifndef __CONSTRAINTS_H
#define __CONSTRAINTS_H

#include <anetlist/net.h>

#include "resmgr.h"

enum {
	CONSTRAINT_NONE,		/* < no constraint */
	CONSTRAINT_SAME_SLICE,		/* < instance must be in the same slice as instance <cobj> */
	CONSTRAINT_ABOVE_SLICE,		/* < instance must be in the slice above that of instance <cobj> */
	CONSTRAINT_BELOW_SLICE,		/* < instance must be in the slice below that of instance <cobj> */
	CONSTRAINT_LOCKED		/* < instance must be located in BEL <cobj> */
};

struct constraint {
	int ctype;			/* < type of constraint, one of CONSTRAINT_* */
	void *cobj;			/* < object the constraint relates to */
	struct resmgr_bel *current;	/* < current placement, NULL if unplaced */
};

void constraints_init(struct anetlist *a);
void constraints_lock(struct anetlist_instance *inst, struct resmgr_bel *bel);
void constraints_infer_rel(struct anetlist *a);
void constraints_free(struct anetlist *a);

#endif /* __CONSTRAINTS_H */
