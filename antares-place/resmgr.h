#ifndef __RESMGR_H
#define __RESMGR_H

#include <anetlist/net.h>
#include <chip/db.h>
#include <mtwist/mtwist.h>

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

enum {
	RESMGR_SITE_SLICEX,
	RESMGR_SITE_SLICEL,
	RESMGR_SITE_SLICEM,
	RESMGR_SITE_TIEOFF,
	RESMGR_SITE_RAMB8BWER,
	RESMGR_SITE_RAMB16BWER,
	RESMGR_SITE_DSP48A1,
	RESMGR_SITE_IOBM,
	RESMGR_SITE_IOBS,
	RESMGR_SITE_ILOGIC2,
	RESMGR_SITE_OLOGIC2,
	RESMGR_SITE_IODELAY2,
	RESMGR_SITE_BUFH,
	RESMGR_SITE_BUFGMUX,
};

int resmgr_get_site_index(const char *name);

enum {
	/* SLICEX/SLICEL/SLICEM */
	RESMGR_BEL_LUT_A,
	RESMGR_BEL_LUT_B,
	RESMGR_BEL_LUT_C,
	RESMGR_BEL_LUT_D,
	RESMGR_BEL_FD_A5,
	RESMGR_BEL_FD_A6,
	RESMGR_BEL_FD_B5,
	RESMGR_BEL_FD_B6,
	RESMGR_BEL_FD_C5,
	RESMGR_BEL_FD_C6,
	RESMGR_BEL_FD_D5,
	RESMGR_BEL_FD_D6,
	RESMGR_BEL_MUXF7_AB,
	RESMGR_BEL_MUXF7_CD,
	RESMGR_BEL_MUXF8,
	RESMGR_BEL_CARRY4,
	/* IOBM */
	RESMGR_BEL_IOBM,
	/* IOBS */
	RESMGR_BEL_IOBS,
};

const char *resmgr_get_bel_name(int bel_offset);

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
	mt_state *prng;					/* < Mersenne twister PRNG */
	int n_control_sets;				/* < number of unique control sets */
	struct resmgr_control_set *control_sets;	/* < description of the control sets */
	struct rtree_node *free_lut;			/* < available LUT6_2s */
	struct rtree_node *free_muxf7;			/* < available MUXF7s */
	struct rtree_node *free_muxf8;			/* < available MUXF8s */
	struct rtree_node *free_fd;			/* < available FDs that are not bound to a control set */
	struct rtree_node **free_fd_cs;			/* < for each control set, list of available FDs bound to it */
	struct rtree_node *free_carry4;			/* < available CARRY4s */
	struct rtree_node *free_iobm;			/* < available IOBMs */
	struct rtree_node *free_iobs;			/* < available IOBSs */
	struct rtree_node *used_resources;		/* < all used resources, non-locked */
	struct rtree_node *used_resources_locked;	/* < all used resources, locked */
};

struct resmgr *resmgr_new(struct anetlist *a, struct db *db);
void resmgr_free(struct resmgr *r);

void resmgr_place(struct resmgr *r, struct anetlist_instance *inst, struct resmgr_bel *to);

#endif /* __RESMGR_H */
