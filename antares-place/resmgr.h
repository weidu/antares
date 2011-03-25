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

int resmgr_cmp_control_sets(struct resmgr_control_set *cs1, struct resmgr_control_set *cs2);

enum {
	RESMGR_SITE_SLICEX,
	RESMGR_SITE_SLICEL,
	RESMGR_SITE_SLICEM,
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

enum {
	RESMGR_BEL_SLICE_LUTA,
	RESMGR_BEL_SLICE_LUTB,
	RESMGR_BEL_SLICE_LUTC,
	RESMGR_BEL_SLICE_LUTD,
	RESMGR_BEL_SLICE_FF5A,
	RESMGR_BEL_SLICE_FF5B,
	RESMGR_BEL_SLICE_FF5C,
	RESMGR_BEL_SLICE_FF5D,
	RESMGR_BEL_SLICE_FF6A,
	RESMGR_BEL_SLICE_FF6B,
	RESMGR_BEL_SLICE_FF6C,
	RESMGR_BEL_SLICE_FF6D,
	RESMGR_BEL_SLICE_MUXF7AB,
	RESMGR_BEL_SLICE_MUXF7CD,
	RESMGR_BEL_SLICE_MUXF8,
	RESMGR_BEL_SLICE_CARRY4,
};

#define RESMGR_BEL_COUNT_SLICEX (RESMGR_BEL_SLICE_FF6D+1)
#define RESMGR_BEL_COUNT_SLICELM (RESMGR_BEL_SLICE_CARRY4+1)

struct resmgr_site {
	struct rtree_leaf_header rlh;		/* < rtree housekeeping */
	int tile;				/* < offset in the chip DB tile array */
	int site_offset;			/* < site offset inside the tile */
	struct anetlist_instance **inst;	/* < netlist instance(s) placed in this site (indexed by RESMGR_BEL_*) */
};

/* Slice state. Slices resources (LUT/FF/X) are filled in this order: BACD. */
struct resmgr_slice_state {
	int used_lut; /* < Used slice LUTs, with or without their associated O5 flip flops. */
	int used_ff6; /* < Used O6 flip flops or X signals */
	int used_carry; /* < 1 if slice is in carry chain mode. This implies used_lut=4. */
};

#define RESMGR_SLICE_STATE_COUNT (5*4+5)

struct resmgr_control_set_resources {
	struct rtree_node **slicex; /* < table of the list of SLICEX in each state */
	struct rtree_node **slicel; /* < table of the list of SLICEL in each state */
	struct rtree_node **slicem; /* < table of the list of SLICEM in each state */
};

struct resmgr {
	struct anetlist *a;				/* < associated netlist */
	struct db *db;					/* < associated chip database */
	mt_state *prng;					/* < Mersenne Twister PRNG state */
	int n_control_sets;				/* < number of unique control sets */
	struct resmgr_control_set *control_sets;	/* < description of the control sets */
	struct resmgr_control_set_resources slices;	/* < slices not bound to a particular control set */
	struct resmgr_control_set_resources *slices_cs;	/* < slices bound to a particular control set */
	struct rtree_node *free_iobm;			/* < available IOBMs */
	struct rtree_node *free_iobs;			/* < available IOBSs */
	struct rtree_node *used_resources;		/* < all used non-slice resources */
};

struct resmgr *resmgr_new(struct anetlist *a, struct db *db);
void resmgr_free(struct resmgr *r);

int resmgr_find_control_set(struct resmgr *r, struct anetlist_instance *inst);
int resmgr_encode_slice_state(struct resmgr_slice_state *ss);

int resmgr_get_site_index(const char *name);
char *resmgr_get_site_name(struct resmgr *r, int tile_index, int site_offset);
int resmgr_get_site_index2(struct resmgr *r, int tile_index, int site_offset);
int resmgr_get_site_index3(struct resmgr *r, struct resmgr_site *s);

#define RESMGR_MAX_POOLS (3*RESMGR_SLICE_STATE_COUNT)

int resmgr_get_slice_pools(struct resmgr *r, int control_set, struct resmgr_slice_state *combine, struct rtree_node **pools);
struct resmgr_site *resmgr_find_carry_in_tile(struct resmgr *r, int i);
struct resmgr_site **resmgr_get_carry_starts(struct resmgr *r, int height, int *n);

void resmgr_place(struct resmgr *r, struct anetlist_instance *inst, struct resmgr_site *site, int bel_index);
void resmgr_unplace(struct resmgr *r, struct anetlist_instance *inst);

#endif /* __RESMGR_H */
