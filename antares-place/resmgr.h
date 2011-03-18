#ifndef __RESMGR_H
#define __RESMGR_H

#include <anetlist/net.h>
#include <chip/db.h>

struct resmgr_net_driver {
	struct anetlist_instance *inst;
	int output;
};

struct resmgr_control_set {
	struct resmgr_net_driver clock;
	struct resmgr_net_driver ce;
	struct resmgr_net_driver sr;
};

struct resmgr {
	int n_control_sets;
	struct resmgr_control_set *control_sets;
};

struct resmgr *resmgr_new(struct anetlist *a, struct db *db);
void resmgr_free(struct resmgr *r);

#endif /* __RESMGR_H */
