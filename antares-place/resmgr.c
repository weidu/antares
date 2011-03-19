#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <anetlist/net.h>
#include <anetlist/entities.h>
#include <anetlist/bels.h>
#include <chip/db.h>

#include "rtree.h"
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

static int get_site_index(const char *name)
{
	if(strcmp(name, "SLICEX") == 0) return RESMGR_SITE_SLICEX;
	if(strcmp(name, "SLICEL") == 0) return RESMGR_SITE_SLICEL;
	if(strcmp(name, "SLICEM") == 0) return RESMGR_SITE_SLICEM;
	if(strcmp(name, "TIEOFF") == 0) return RESMGR_SITE_TIEOFF;
	if(strcmp(name, "RAMB8BWER") == 0) return RESMGR_SITE_RAMB8BWER;
	if(strcmp(name, "RAMB16BWER") == 0) return RESMGR_SITE_RAMB16BWER;
	if(strcmp(name, "DSP48A1") == 0) return RESMGR_SITE_DSP48A1;
	if(strcmp(name, "IOBM") == 0) return RESMGR_SITE_IOBM;
	if(strcmp(name, "IOBS") == 0) return RESMGR_SITE_IOBS;
	if(strcmp(name, "ILOGIC2") == 0) return RESMGR_SITE_ILOGIC2;
	if(strcmp(name, "OLOGIC2") == 0) return RESMGR_SITE_OLOGIC2;
	if(strcmp(name, "IODELAY2") == 0) return RESMGR_SITE_IODELAY2;
	if(strcmp(name, "BUFH") == 0) return RESMGR_SITE_BUFH;
	if(strcmp(name, "BUFGMUX") == 0) return RESMGR_SITE_BUFGMUX;
	return -1;
}

static void add_bel(struct resmgr *r, struct rtree_node *root, int type, int tile, int site_offset, int bel_offset)
{
	struct resmgr_bel *bel;
	struct tile *t;
	
	t = &r->db->chip.tiles[tile];
	
	bel = alloc_type(struct resmgr_bel);
	bel->type = type;
	bel->tile = tile;
	bel->site_offset = site_offset;
	bel->bel_offset = bel_offset;
	bel->inst = t->user;
	t->user = bel;
	rtree_add(root, bel);
}

static void populate_resources(struct resmgr *r, struct db *db)
{
	int i, j, k;
	struct tile *tile;
	struct tile_type *tile_type;
	char *site_name;
	int site_index;
	
	for(i=0;i<db->chip.w*db->chip.h;i++) {
		tile = &db->chip.tiles[i];
		tile_type = &db->tile_types[tile->type];
		for(j=0;j<tile_type->n_sites;j++) {
			site_name = db->site_types[tile_type->sites[j]].name;
			site_index = get_site_index(site_name);
			switch(site_index) {
				case RESMGR_SITE_SLICEM:
					/* fall through */
				case RESMGR_SITE_SLICEL:
					add_bel(r, r->free_carry4, ANETLIST_BEL_CARRY4, i, j, 0);
					/* fall through */
				case RESMGR_SITE_SLICEX:
					for(k=0;k<4;k++)
						add_bel(r, r->free_lut, ANETLIST_BEL_LUT6_2, i, j, k);
					for(k=0;k<8;k++)
						add_bel(r, r->free_fd, ANETLIST_BEL_FDRE, i, j, k);
					break;
				case RESMGR_SITE_IOBM:
					add_bel(r, r->free_iobm, ANETLIST_BEL_IOBM, i, j, 0);
					break;
				case RESMGR_SITE_IOBS:
					add_bel(r, r->free_iobs, ANETLIST_BEL_IOBS, i, j, 0);
					break;
				default:
					break;
			}
		}
	}
}

struct resmgr *resmgr_new(struct anetlist *a, struct db *db)
{
	struct resmgr *r;
	int i;
	
	r = alloc_type(struct resmgr);
	
	r->a = a;
	r->db = db;
	
	r->n_control_sets = 0;
	r->control_sets = NULL;
	populate_control_sets(r, a);
	printf("Number of unique control sets:\t%d\n", r->n_control_sets);
	
	r->free_lut = rtree_new_root();
	r->free_fd = rtree_new_root();
	r->free_fd_cs = alloc_size(r->n_control_sets*sizeof(struct rtree_node *));
	for(i=0;i<r->n_control_sets;i++)
		r->free_fd_cs[i] = rtree_new_root();
	r->free_carry4 = rtree_new_root();
	r->free_iobm = rtree_new_root();
	r->free_iobs = rtree_new_root();
	r->used_resources = rtree_new_root();
	populate_resources(r, db);
	printf("Available LUTs:\t\t\t%d\n", r->free_lut->count);
	printf("Available FDs:\t\t\t%d\n", r->free_fd->count);
	printf("Available CARRY4s:\t\t%d\n", r->free_carry4->count);
	printf("Available IOBMs:\t\t%d\n", r->free_iobm->count);
	printf("Available IOBSs:\t\t%d\n", r->free_iobs->count);
	
	return r;
}

void resmgr_free(struct resmgr *r)
{
	int i;
	
	rtree_free(r->free_lut, free);
	rtree_free(r->free_fd, free);
	for(i=0;i<r->n_control_sets;i++)
		rtree_free(r->free_fd_cs[i], free);
	free(r->free_fd_cs);
	rtree_free(r->free_carry4, free);
	rtree_free(r->free_iobm, free);
	rtree_free(r->free_iobs, free);
	rtree_free(r->used_resources, free);
	free(r->control_sets);
	free(r);
}
