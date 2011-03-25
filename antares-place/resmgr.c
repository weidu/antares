#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <anetlist/net.h>
#include <anetlist/entities.h>
#include <anetlist/bels.h>
#include <chip/db.h>
#include <mtwist/mtwist.h>

#include "rtree.h"
#include "constraints.h"
#include "resmgr.h"

int resmgr_cmp_control_sets(struct resmgr_control_set *cs1, struct resmgr_control_set *cs2)
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
		if(resmgr_cmp_control_sets(&r->control_sets[i], cs))
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

int resmgr_get_site_index(const char *name)
{
	if(strcmp(name, "SLICEX") == 0) return RESMGR_SITE_SLICEX;
	if(strcmp(name, "SLICEL") == 0) return RESMGR_SITE_SLICEL;
	if(strcmp(name, "SLICEM") == 0) return RESMGR_SITE_SLICEM;
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

char *resmgr_get_site_name(struct resmgr *r, int tile_index, int site_offset)
{
	struct tile *tile;
	struct tile_type *tile_type;

	tile = &r->db->chip.tiles[tile_index];
	tile_type = &r->db->tile_types[tile->type];
	return r->db->site_types[tile_type->sites[site_offset]].name;
}

int resmgr_get_site_index2(struct resmgr *r, int tile_index, int site_offset)
{
	return resmgr_get_site_index(resmgr_get_site_name(r, tile_index, site_offset));
}

int resmgr_get_site_index3(struct resmgr *r, struct resmgr_site *s)
{
	return resmgr_get_site_index2(r, s->tile, s->site_offset);
}

static void add_site(struct resmgr *r, struct rtree_node *root, int tile, int site_offset, int bel_count)
{
	struct resmgr_site *site;
	struct tile *t;
	struct resmgr_site **tile_sites;
	
	t = &r->db->chip.tiles[tile];
	
	site = alloc_type(struct resmgr_site);
	site->tile = tile;
	site->site_offset = site_offset;
	site->inst = alloc_size0(bel_count*sizeof(struct anetlist_instance *));
	tile_sites = t->user;
	tile_sites[site_offset] = site;
	rtree_add(root, site);
}

static void free_site(void *_site)
{
	struct resmgr_site *site = _site;
	free(site->inst);
	free(site);
}

static void populate_resources(struct resmgr *r)
{
	int i, j;
	struct tile *tile;
	struct tile_type *tile_type;
	char *site_name;
	int site_index;
	
	for(i=0;i<r->db->chip.w*r->db->chip.h;i++) {
		tile = &r->db->chip.tiles[i];
		tile_type = &r->db->tile_types[tile->type];
		tile->user = alloc_size(tile_type->n_sites*sizeof(struct resmgr_site *));
		for(j=0;j<tile_type->n_sites;j++) {
			site_name = r->db->site_types[tile_type->sites[j]].name;
			site_index = resmgr_get_site_index(site_name);
			switch(site_index) {
				case RESMGR_SITE_SLICEX:
					add_site(r, r->slices.slicex[0], i, j, RESMGR_BEL_COUNT_SLICEX);
					break;
				case RESMGR_SITE_SLICEL:
					add_site(r, r->slices.slicel[0], i, j, RESMGR_BEL_COUNT_SLICELM);
					break;
				case RESMGR_SITE_SLICEM:
					add_site(r, r->slices.slicem[0], i, j, RESMGR_BEL_COUNT_SLICELM);
					break;
				case RESMGR_SITE_IOBM:
					add_site(r, r->free_iobm, i, j, 1);
					break;
				case RESMGR_SITE_IOBS:
					add_site(r, r->free_iobs, i, j, 1);
					break;
				default:
					break;
			}
		}
	}
}

static void alloc_csr(struct resmgr_control_set_resources *csr)
{
	int i;
	
	csr->slicex = alloc_size(RESMGR_SLICE_STATE_COUNT*sizeof(struct rtree_node *));
	for(i=0;i<RESMGR_SLICE_STATE_COUNT;i++)
		csr->slicex[i] = rtree_new_root();
	csr->slicel = alloc_size(RESMGR_SLICE_STATE_COUNT*sizeof(struct rtree_node *));
	for(i=0;i<RESMGR_SLICE_STATE_COUNT;i++)
		csr->slicel[i] = rtree_new_root();
	csr->slicem = alloc_size(RESMGR_SLICE_STATE_COUNT*sizeof(struct rtree_node *));
	for(i=0;i<RESMGR_SLICE_STATE_COUNT;i++)
		csr->slicem[i] = rtree_new_root();
}

static void free_csr(struct resmgr_control_set_resources *csr)
{
	int i;
	
	for(i=0;i<RESMGR_SLICE_STATE_COUNT;i++)
		rtree_free(csr->slicex[i], free_site);
	free(csr->slicex);
	for(i=0;i<RESMGR_SLICE_STATE_COUNT;i++)
		rtree_free(csr->slicel[i], free_site);
	free(csr->slicel);
	for(i=0;i<RESMGR_SLICE_STATE_COUNT;i++)
		rtree_free(csr->slicem[i], free_site);
	free(csr->slicem);
}

struct resmgr *resmgr_new(struct anetlist *a, struct db *db)
{
	struct resmgr *r;
	int i;
	
	r = alloc_type(struct resmgr);
	
	r->a = a;
	r->db = db;
	r->prng = alloc_type0(mt_state);
	mts_seed32(r->prng, 0);
	
	r->n_control_sets = 0;
	r->control_sets = NULL;
	populate_control_sets(r, a);
	printf("Number of unique control sets:\t%d\n", r->n_control_sets);
	
	alloc_csr(&r->slices);
	r->slices_cs = alloc_size(r->n_control_sets*sizeof(struct resmgr_control_set_resources *));
	for(i=0;i<r->n_control_sets;i++)
		alloc_csr(&r->slices_cs[i]);
	r->free_iobm = rtree_new_root();
	r->free_iobs = rtree_new_root();
	r->used_resources = rtree_new_root();
	populate_resources(r);
	printf("Available SLICEXs:\t\t%d\n", r->slices.slicex[0]->count);
	printf("Available SLICELs:\t\t%d\n", r->slices.slicel[0]->count);
	printf("Available SLICEMs:\t\t%d\n", r->slices.slicem[0]->count);
	printf("Available IOBMs:\t\t%d\n", r->free_iobm->count);
	printf("Available IOBSs:\t\t%d\n", r->free_iobs->count);
	
	return r;
}

void resmgr_free(struct resmgr *r)
{
	int i;
	
	free(r->prng);
	for(i=0;i<r->db->chip.w*r->db->chip.h;i++)
		free(r->db->chip.tiles[i].user);
	free_csr(&r->slices);
	for(i=0;i<r->n_control_sets;i++)
		free_csr(&r->slices_cs[i]);
	free(r->slices_cs);
	rtree_free(r->free_iobm, free_site);
	rtree_free(r->free_iobs, free_site);
	rtree_free(r->used_resources, free_site);
	free(r->control_sets);
	free(r);
}

int resmgr_find_control_set(struct resmgr *r, struct anetlist_instance *inst)
{
	struct resmgr_control_set cs;
	int i;

	if(inst->e != &anetlist_bels[ANETLIST_BEL_FDRE])
		return -1;
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
	for(i=0;i<r->n_control_sets;i++)
		if(resmgr_cmp_control_sets(&cs, &r->control_sets[i]))
			return i;
	return -2;
}

int resmgr_encode_slice_state(struct resmgr_slice_state *ss)
{
	int r;
	
	assert(ss->used_lut < 5);
	assert(ss->used_ff6 < 5);
	r = ss->used_lut*5+ss->used_ff6;
	if(ss->used_carry) {
		assert(ss->used_lut == 4);
		r += 5;
	}
	assert(r < RESMGR_SLICE_STATE_COUNT);
	return r;
}

int resmgr_get_slice_pools(struct resmgr *r, int control_set, struct resmgr_slice_state *combine, struct rtree_node **pools)
{
	struct resmgr_control_set_resources *resources;
	int count;
	int max_used_lut, max_used_ff6;
	int i, j;
	struct resmgr_slice_state ss;
	int se;
	
	if(control_set < 0)
		resources = &r->slices;
	else
		resources = &r->slices_cs[control_set];
	
	count = 0;
	max_used_lut = 4 - combine->used_lut;
	max_used_ff6 = 4 - combine->used_ff6;
	for(i=0;i<max_used_ff6;i++)
		for(j=0;j<max_used_lut;j++) {
			ss.used_lut = i;
			ss.used_ff6 = j;
			if(combine->used_carry) {
				ss.used_carry = 0;
				se = resmgr_encode_slice_state(&ss);
				pools[count++] = resources->slicel[se];
				pools[count++] = resources->slicem[se];
			} else {
				ss.used_carry = 0;
				se = resmgr_encode_slice_state(&ss);
				pools[count++] = resources->slicex[se];
				pools[count++] = resources->slicel[se];
				pools[count++] = resources->slicem[se];
				ss.used_carry = 1;
				se = resmgr_encode_slice_state(&ss);
				pools[count++] = resources->slicel[se];
				pools[count++] = resources->slicem[se];
			}
		}
	
	return count;
}

struct resmgr_site *resmgr_find_carry_in_tile(struct resmgr *r, int i)
{
	int j, k;
	struct tile *tile;
	struct tile_type *tile_type;
	struct resmgr_site **tile_rs;
	char *site_name;
	int site_index;

	if((i < 0)||(i >= r->db->chip.w*r->db->chip.h))
		return NULL;
	tile = &r->db->chip.tiles[i];
	tile_rs = tile->user;
	tile_type = &r->db->tile_types[tile->type];
	for(j=0;j<tile_type->n_sites;j++) {
		site_name = r->db->site_types[tile_type->sites[j]].name;
		site_index = resmgr_get_site_index(site_name);
		if((site_index == RESMGR_SITE_SLICEL)||(site_index == RESMGR_SITE_SLICEM)) {
			for(k=0;k<RESMGR_BEL_COUNT_SLICELM;k++)
				if(tile_rs[j]->inst[k] != NULL)
					return NULL; /* only accept fully empty slices */
			return tile_rs[j];
		}
	}
	return NULL;
}

static struct resmgr_site *carry_possible(struct resmgr *r, int i, int height)
{
	struct resmgr_site *s, *s2;

	s = NULL;
	while(height > 0) {
		s2 = resmgr_find_carry_in_tile(r, i);
		if(s2 == NULL)
			return NULL;
		if(s == NULL)
			s = s2;
		height--;
		i -= r->db->chip.w;
	}
	return s;
}

struct resmgr_site **resmgr_get_carry_starts(struct resmgr *r, int height, int *n)
{
	struct resmgr_site **list;
	int list_alloc;
	int i;
	struct resmgr_site *s;
	
	*n = 0;
	list_alloc = 128;
	list = alloc_size(list_alloc*sizeof(struct resmgr_site *));
	
	for(i=0;i<r->db->chip.w*r->db->chip.h;i++) {
		s = carry_possible(r, i, height);
		if(s != NULL) {
			if(*n == list_alloc) {
				list_alloc *= 2;
				list = realloc(list, list_alloc*sizeof(struct resmgr_site *));
				if(list == NULL) abort();
			}
			list[*n] = s;
			(*n)++;
		}
	}
	
	return list;
}

static void return_slice(struct resmgr *r, struct resmgr_site *site, int site_type)
{
	int control_set;
	struct resmgr_slice_state ss;
	int ssi;
	int i;
	int x;
	struct resmgr_control_set_resources *csr;
	
	control_set = -1;
	for(i=RESMGR_BEL_SLICE_FF5A;i<=RESMGR_BEL_SLICE_FF6D;i++) {
		if(site->inst[i] != NULL) {
			x = resmgr_find_control_set(r, site->inst[i]);
			assert(x >= 0);
			if(control_set == -1)
				control_set = x;
			else {
				assert(control_set == x);
			}
		}
	}
	
	ss.used_lut = 0;
	for(i=RESMGR_BEL_SLICE_LUTA;i<=RESMGR_BEL_SLICE_LUTD;i++)
		if(site->inst[i] != NULL)
			ss.used_lut++;
	ss.used_ff6 = 0;
	for(i=RESMGR_BEL_SLICE_FF6A;i<=RESMGR_BEL_SLICE_MUXF8;i++)
		if(site->inst[i] != NULL)
			ss.used_ff6++;
	if(site->inst[RESMGR_BEL_SLICE_CARRY4] != NULL)
		ss.used_carry = 1;
	else
		ss.used_carry = 0;
	ssi = resmgr_encode_slice_state(&ss);
	
	if(control_set == -1)
		csr = &r->slices;
	else
		csr = &r->slices_cs[control_set];
	switch(site_type) {
		case RESMGR_SITE_SLICEX:
			rtree_add(csr->slicex[ssi], site);
			break;
		case RESMGR_SITE_SLICEL:
			rtree_add(csr->slicel[ssi], site);
			break;
		case RESMGR_SITE_SLICEM:
			rtree_add(csr->slicem[ssi], site);
			break;
		default:
			assert(0);
			break;
	}
}

void resmgr_place(struct resmgr *r, struct anetlist_instance *inst, struct resmgr_site *site, int bel_index)
{
	int site_type;
	struct constraint *constraint;
	
	printf("Placing instance %s:%s to %s:%s (BEL index %d)\n",
		inst->uid, inst->e->name,
		r->db->chip.tiles[site->tile].sites[site->site_offset].name,
		resmgr_get_site_name(r, site->tile, site->site_offset),
		bel_index);
	
	assert(site->inst[bel_index] == NULL);
	site->inst[bel_index] = inst;
	constraint = inst->user;
	constraint->current = site;
	constraint->current_bel_index = bel_index;
	
	site_type = resmgr_get_site_index3(r, site);
	rtree_del(site);
	switch(site_type) {
		case RESMGR_SITE_SLICEX:
		case RESMGR_SITE_SLICEL:
		case RESMGR_SITE_SLICEM:
			return_slice(r, site, site_type);
			break;
		default:
			rtree_add(r->used_resources, site);
			break;
	}
}

void resmgr_unplace(struct resmgr *r, struct anetlist_instance *inst)
{
	/* TODO */
}
