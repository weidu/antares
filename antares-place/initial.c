#include <assert.h>
#include <stdlib.h>

#include <anetlist/net.h>
#include <anetlist/entities.h>
#include <anetlist/bels.h>

#include <mtwist/mtwist.h>

#include "resmgr.h"
#include "rtree.h"
#include "constraints.h"
#include "initial.h"

void *rtrees_random_pick(struct rtree_node **roots, int n_roots, unsigned int rnd)
{
	int total_count;
	int i;
	
	total_count = 0;
	for(i=0;i<n_roots;i++)
		total_count += roots[i]->count;
	if(total_count == 0)
		return NULL;
	rnd %= total_count;
	i = 0;
	while(rnd >= roots[i]->count) {
		rnd -= roots[i]->count;
		i++;
	}
	return rtree_get(roots[i], rnd);
}

static int get_bel_index(struct anetlist_entity *e)
{
	int len;
	int index;
	
	len = sizeof(anetlist_bels)/sizeof(anetlist_bels[0]);
	index = e - anetlist_bels;
	if(index >= len)
		index = -1;
	return index;
}

static void place_locked(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;

	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL) && (constraint->lock != NULL))
			resmgr_place(r, inst, constraint->lock, constraint->lock_bel_index);
		inst = inst->next;
	}
}

static int is_fd5(struct anetlist_instance *inst)
{
	struct anetlist_instance *driving_inst;
	int driving_pin;
	struct constraint *constraint, *driving_constraint;

	driving_inst = inst->inputs[ANETLIST_BEL_FDRE_D]->inst;
	driving_pin = inst->inputs[ANETLIST_BEL_FDRE_D]->pin;
	if((driving_inst->e != &anetlist_bels[ANETLIST_BEL_LUT6_2]) || (driving_pin != ANETLIST_BEL_LUT6_2_O5))
		return 0;
	constraint = inst->user;
	driving_constraint = driving_inst->user;
	if(driving_constraint->group != constraint->group)
		return 0;
	return 1;
}

struct group_enum {
	int lut_count, muxf7_count, muxf8_count, fd5_count, fd6_count, carry4_count;
	struct anetlist_instance *lut[4], *muxf7[2], *muxf8[1], *fd5[4], *fd6[4], *carry4[1];
};

static int enumerate_group(struct group_enum *e, struct constraint_group *g)
{
	int bel;

	e->lut_count = 0;
	e->muxf7_count = 0;
	e->muxf8_count = 0;
	e->fd5_count = 0;
	e->fd6_count = 0;
	e->carry4_count = 0;
	
	while(g != NULL) {
		bel = get_bel_index(g->inst->e);
		switch(bel) {
			case ANETLIST_BEL_LUT6_2:
				if(e->lut_count == 4) {
					fprintf(stderr, "More than 4 LUTs in the same slice (instance %s)\n",
						g->inst->uid);
					exit(EXIT_FAILURE);
				}
				e->lut[e->lut_count++] = g->inst;
				break;
			case ANETLIST_BEL_MUXF7:
				if(e->muxf7_count == 2) {
					fprintf(stderr, "More than 2 MUXF7s in the same slice (instance %s)\n",
						g->inst->uid);
					exit(EXIT_FAILURE);
				}
				e->muxf7[e->muxf7_count++] = g->inst;
				break;
			case ANETLIST_BEL_MUXF8:
				if(e->muxf8_count == 1) {
					fprintf(stderr, "More than 1 MUXF8s in the same slice (instance %s)\n",
						g->inst->uid);
					exit(EXIT_FAILURE);
				}
				e->muxf8[e->muxf8_count++] = g->inst;
				break;
			case ANETLIST_BEL_FDRE:
				if(is_fd5(g->inst)) {
					if(e->fd5_count == 4) {
						fprintf(stderr, "More than 4 FD5s in the same slice (instance %s)\n",
							g->inst->uid);
						exit(EXIT_FAILURE);
					}
					e->fd5[e->fd5_count++] = g->inst;
				} else {
					if(e->fd6_count == 4) {
						fprintf(stderr, "More than 4 FD6s in the same slice (instance %s)\n",
							g->inst->uid);
						exit(EXIT_FAILURE);
					}
					e->fd6[e->fd6_count++] = g->inst;
				}
				break;
			case ANETLIST_BEL_CARRY4:
				if(e->carry4_count == 1) {
					fprintf(stderr, "More than 1 CARRY4s in the same slice (instance %s)\n",
						g->inst->uid);
					exit(EXIT_FAILURE);
				}
				e->carry4[e->carry4_count++] = g->inst;
				break;
			default:
				return 0;
		}
		g = g->next;
	}
	return 1;
}

static void place_slice(struct resmgr *r, struct anetlist_instance *inst, struct resmgr_site *s)
{
	struct constraint *constraint;
	struct constraint_group group_single;
	struct constraint_group *g;
	struct group_enum e;
	int control_set;
	int x;
	struct resmgr_slice_state combine;
	int existing;
	int i;

	/* enumerate group */
	constraint = inst->user;
	g = constraint->group;
	if(g == NULL) {
		group_single.inst = inst;
		group_single.next = NULL;
		g = &group_single;
	}
	if(!enumerate_group(&e, g))
		return;
	combine.used_lut = e.lut_count;
	if((e.muxf8_count == 1)||(e.muxf7_count == 2))
		combine.used_ff6 = 3;
	else if(e.muxf7_count == 1)
		combine.used_ff6 = 1;
	else
		combine.used_ff6 = e.fd6_count;
	combine.used_carry = e.carry4_count;
	
	/* find synchronous control set (if any) */
	control_set = -1;
	constraint = inst->user;
	g = constraint->group;
	while(g != NULL) {
		x = resmgr_find_control_set(r, g->inst);
		assert(x != -2);
		if(x >= 0) {
			if(control_set == -1)
				control_set = x;
			else if(control_set != x) {
				fprintf(stderr, "Elements with incompatible control sets grouped in the same slice\n");
				exit(EXIT_FAILURE);
			}
		}
		g = g->next;
	}
	
	/* if site is not specified, pick one */
	if(s == NULL) {
		struct rtree_node *pools[2*RESMGR_MAX_POOLS];
		int npools;
		int allow_x;
		
		allow_x = (e.muxf7_count == 0) && (e.muxf8_count == 0);
		npools = resmgr_get_slice_pools(r, control_set, &combine, allow_x, pools);
		if(control_set != -1)
			npools += resmgr_get_slice_pools(r, -1, &combine, allow_x, &pools[npools]);
		s = rtrees_random_pick(pools, npools, mts_lrand(r->prng));
		if(s == NULL) {
			fprintf(stderr, "Failed to find slice for LUT=%d FF6=%d CARRY=%d\n",
				combine.used_lut, combine.used_ff6, combine.used_carry);
			exit(EXIT_FAILURE);
		}
	}
	
	if(e.muxf8_count) {
		assert(e.muxf7_count == 2);
		assert(!e.carry4_count);
		resmgr_place(r, e.muxf8[0]->inputs[ANETLIST_BEL_MUXF8_I1]->inst->inputs[ANETLIST_BEL_MUXF7_I0]->inst,
			s, RESMGR_BEL_SLICE_LUTB);
		resmgr_place(r, e.muxf8[0]->inputs[ANETLIST_BEL_MUXF8_I1]->inst->inputs[ANETLIST_BEL_MUXF7_I1]->inst,
			s, RESMGR_BEL_SLICE_LUTA);
		resmgr_place(r, e.muxf8[0]->inputs[ANETLIST_BEL_MUXF8_I1]->inst, s, RESMGR_BEL_SLICE_MUXF7AB);
		resmgr_place(r, e.muxf8[0]->inputs[ANETLIST_BEL_MUXF8_I0]->inst->inputs[ANETLIST_BEL_MUXF7_I0]->inst,
			s, RESMGR_BEL_SLICE_LUTD);
		resmgr_place(r, e.muxf8[0]->inputs[ANETLIST_BEL_MUXF8_I0]->inst->inputs[ANETLIST_BEL_MUXF7_I1]->inst,
			s, RESMGR_BEL_SLICE_LUTC);
		resmgr_place(r, e.muxf8[0]->inputs[ANETLIST_BEL_MUXF8_I0]->inst, s, RESMGR_BEL_SLICE_MUXF7CD);
		resmgr_place(r, e.muxf8[0], s, RESMGR_BEL_SLICE_MUXF8);
	} else if(e.muxf7_count) {
		assert(e.muxf7_count == 1);
		assert(!e.carry4_count);
		if((s->inst[RESMGR_BEL_SLICE_LUTA] != NULL)||(s->inst[RESMGR_BEL_SLICE_LUTB] != NULL)) {
			resmgr_place(r, e.muxf7[0]->inputs[ANETLIST_BEL_MUXF7_I0]->inst, s, RESMGR_BEL_SLICE_LUTD);
			resmgr_place(r, e.muxf7[0]->inputs[ANETLIST_BEL_MUXF7_I1]->inst, s, RESMGR_BEL_SLICE_LUTC);
			resmgr_place(r, e.muxf7[0], s, RESMGR_BEL_SLICE_MUXF7CD);
		} else {
			resmgr_place(r, e.muxf7[0]->inputs[ANETLIST_BEL_MUXF7_I0]->inst, s, RESMGR_BEL_SLICE_LUTB);
			resmgr_place(r, e.muxf7[0]->inputs[ANETLIST_BEL_MUXF7_I1]->inst, s, RESMGR_BEL_SLICE_LUTA);
			resmgr_place(r, e.muxf7[0], s, RESMGR_BEL_SLICE_MUXF7AB);
		}
	} else if(e.carry4_count) {
		if(e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S1] != NULL)
			resmgr_place(r, e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S1]->inst, s, RESMGR_BEL_SLICE_LUTB);
		if(e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S0] != NULL)
			resmgr_place(r, e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S0]->inst, s, RESMGR_BEL_SLICE_LUTA);
		if(e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S3] != NULL)
			resmgr_place(r, e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S3]->inst, s, RESMGR_BEL_SLICE_LUTD);
		if(e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S2] != NULL)
			resmgr_place(r, e.carry4[0]->inputs[ANETLIST_BEL_CARRY4_S2]->inst, s, RESMGR_BEL_SLICE_LUTC);
		resmgr_place(r, e.carry4[0], s, RESMGR_BEL_SLICE_CARRY4);
	} else {
		existing = 0;
		for(i=RESMGR_BEL_SLICE_LUTA;i<=RESMGR_BEL_SLICE_LUTD;i++)
			if(s->inst[i] != NULL)
				existing++;
		for(i=0;i<e.lut_count;i++) {
			switch(existing+i) {
				case 0:
					resmgr_place(r, e.lut[i], s, RESMGR_BEL_SLICE_LUTB);
					break;
				case 1:
					resmgr_place(r, e.lut[i], s, RESMGR_BEL_SLICE_LUTA);
					break;
				case 2:
					resmgr_place(r, e.lut[i], s, RESMGR_BEL_SLICE_LUTD);
					break;
				case 3:
					resmgr_place(r, e.lut[i], s, RESMGR_BEL_SLICE_LUTC);
					break;
				default:
					assert(0);
					break;
			}
		}
	}
	for(i=0;i<e.fd5_count;i++) {
		/* TODO */
	}
	for(i=0;i<e.fd6_count;i++) {
		/* TODO */
	}
}

static struct anetlist_instance *get_chain_bottom(struct anetlist_instance *i)
{
	struct anetlist_instance *b;
	
	b = i;
	while(((struct constraint *)b->user)->chain_below != NULL) {
		assert(((struct constraint *)b->user)->chain_below != i);
		b = ((struct constraint *)b->user)->chain_below;
	}
	return b;
}

static int get_chain_height(struct anetlist_instance *b)
{
	int h;
	
	h = 1;
	while(((struct constraint *)b->user)->chain_above != NULL) {
		h++;
		b = ((struct constraint *)b->user)->chain_above;
	}
	return h;
}

static void place_chains(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	struct anetlist_instance *bottom;
	int height;
	struct resmgr_site **chain_starts;
	int n_chain_starts;
	struct resmgr_site *site;
	int tile;
	
	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL)
		  && ((constraint->chain_below != NULL) || (constraint->chain_above == NULL))) {
			bottom = get_chain_bottom(inst);
			height = get_chain_height(bottom);
			chain_starts = resmgr_get_carry_starts(r, height, &n_chain_starts);
			site = chain_starts[mts_lrand(r->prng) % n_chain_starts];
			free(chain_starts);
			tile = site->tile;
			while(height > 0) {
				site = resmgr_find_carry_in_tile(r, tile);
				assert(site != NULL);
				place_slice(r, bottom, site);
				bottom = ((struct constraint *)bottom->user)->chain_above;
				height--;
				tile -= r->db->chip.w;
			}
		}
		inst = inst->next;
	}
}

static void place_slices(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	
	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL) && (constraint->group != NULL))
			place_slice(r, inst, NULL);
		inst = inst->next;
	}
}

static void place_unconstrained(struct resmgr *r)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
	int bel;
	struct rtree_node *roots[2];
	
	inst = r->a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if((constraint->current == NULL)
		  && (constraint->group == NULL)
		  && (constraint->chain_above == NULL)
		  && (constraint->chain_below == NULL)
		  && (constraint->lock == NULL)
		  && (constraint->current == NULL)) {
			bel = get_bel_index(inst->e);
			switch(bel) {
				case ANETLIST_BEL_GND:
				case ANETLIST_BEL_VCC:
				case ANETLIST_BEL_BUFGMUX:
					/* nothing to do */
					break;
				case ANETLIST_BEL_IOBM:
					roots[0] = r->free_iobm;
					roots[1] = r->free_iobs;
					resmgr_place(r, inst, rtrees_random_pick(roots, 2, mts_lrand(r->prng)), 0);
					break;
				default:
					fprintf(stderr, "Unsupported BEL for fully unconstrained placement: %s (instance %s)\n", inst->e->name, inst->uid);
					exit(EXIT_FAILURE);
			}
		}
		inst = inst->next;
	}
}

void initial_placement(struct resmgr *r)
{
	place_locked(r);
	place_chains(r);
	place_slices(r);
	place_unconstrained(r);
}
