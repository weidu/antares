#ifndef __RESMGR_H
#define __RESMGR_H

#include <anetlist/net.h>
#include <chip/db.h>

#include "rtree.h"

struct resmgr_net_driver {
	struct anetlist_instance *inst;
	int output;
};

struct resmgr_control_set {
	struct resmgr_net_driver clock;
	struct resmgr_net_driver ce;
	struct resmgr_net_driver sr;
};

struct resmgr_bel {
	struct rtree_leaf_header rlh;		/* < rtree housekeeping */
	int type;				/* < index in anetlist_bels */
	int tile;				/* < offset in the chip DB tile array */
	int site_offset;			/* < site offset inside the tile */
	int bel_offset;				/* < BEL offset inside the site */
	struct anetlist_instance *inst;		/* < netlist instance cross-reference */
	struct resmgr_bel *next;		/* < next BEL in the tile */
};

struct resmgr {
	struct anetlist *a;				/* < associated netlist */
	struct db *db;					/* < associated chip database */
	int n_control_sets;				/* < number of unique control sets */
	struct resmgr_control_set *control_sets;	/* < description of the control sets */
	struct rtree_node *free_lut;			/* < available LUT6_2s */
	struct rtree_node *free_fd;			/* < available FDs that are not bound to a control set */
	struct rtree_node **free_fd_cs;			/* < for each control set, list of available FDs bound to it */
	struct rtree_node *free_carry4;			/* < available CARRY4s */
	struct rtree_node *free_iobm;			/* < available IOBMs */
	struct rtree_node *free_iobs;			/* < available IOBSs */
	struct rtree_node *used_resources;		/* < all used resources */
};

struct resmgr *resmgr_new(struct anetlist *a, struct db *db);
void resmgr_free(struct resmgr *r);

#endif /* __RESMGR_H */
