#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <anetlist/entities.h>
#include <anetlist/net.h>
#include <anetlist/primitives.h>
#include <anetlist/bels.h>

#include "transform.h"

static int get_primitive_index(struct anetlist_entity *e)
{
	int len;
	int index;
	
	len = sizeof(anetlist_primitives)/sizeof(anetlist_primitives[0]);
	index = e - anetlist_primitives;
	if(index >= len)
		index = -1;
	return index;
}

struct imap {
	struct anetlist_instance *inst;
	int *pins;
};

static struct imap *create_imap(struct anetlist_instance *inst)
{
	struct imap *i;
	
	i = alloc_type(struct imap);
	i->pins = alloc_size(inst->e->n_inputs*sizeof(int));
	assert(inst->user == NULL);
	inst->user = i;
	return i;
}

static void transform_inst(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst);

static void match_output(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *new, int newoutput, struct anetlist_instance *inst, int instoutput)
{
	struct anetlist_endpoint *ep;
	struct imap *i;
	
	ep = inst->outputs[instoutput];
	while(ep != NULL) {
		transform_inst(src, dst, ep->inst);
		i = ep->inst->user;
		if(i->pins[ep->pin] != -1)
			anetlist_connect(new, newoutput, i->inst, i->pins[ep->pin]);
		ep = ep->next;
	}
}

/* TODO: how to set the IOBs in simple input/output mode? */
static void transform_ibuf(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *iobm;
	struct imap *i;
	
	i = create_imap(inst);
	iobm = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_IOBM]);
	i->inst = iobm;
	match_output(src, dst, iobm, ANETLIST_BEL_IOBM_I, inst, ANETLIST_PRIMITIVE_IBUF_O);
}

static void transform_obuf(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *iobm;
	struct imap *i;
	
	i = create_imap(inst);
	iobm = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_IOBM]);
	i->inst = iobm;
	i->pins[ANETLIST_PRIMITIVE_OBUF_I] = ANETLIST_BEL_IOBM_O;
}

static void transform_lut(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *lut6_2;
	struct imap *i;
	int j;
	
	i = create_imap(inst);
	lut6_2 = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_LUT6_2]);
	i->inst = lut6_2;
	for(j=0;j<inst->e->n_inputs;j++)
		i->pins[j] = j;
	strcpy(lut6_2->attributes[0]+16-strlen(inst->attributes[0]), inst->attributes[0]);
	match_output(src, dst, lut6_2, ANETLIST_BEL_LUT6_2_O6, inst, 0);
}

/* TODO: how to set the IOB in simple input mode? */
static void transform_bufgp(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *iobm;
	struct imap *i;

	// TODO: insert BUFG
	i = create_imap(inst);
	iobm = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_IOBM]);
	i->inst = iobm;
	match_output(src, dst, iobm, ANETLIST_BEL_IOBM_I, inst, ANETLIST_PRIMITIVE_BUFGP_O);
}

static struct anetlist_instance *get_driven(struct anetlist_endpoint *ep, struct anetlist_entity *e, int pin)
{
	while(ep != NULL) {
		if((ep->inst->e == e) && (ep->pin == pin))
			return ep->inst;
		ep = ep->next;
	}
	return NULL;
}

static struct anetlist_instance *carrychain_get_bottom(struct anetlist_instance *cce)
{
	struct anetlist_instance *r;
	
	if(cce->e == &anetlist_primitives[ANETLIST_PRIMITIVE_XORCY]) {
		r = cce->inputs[ANETLIST_PRIMITIVE_XORCY_CI]->inst;
		if(r->e != &anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY]) {
			struct anetlist_endpoint *ep;
			
			ep = cce->inputs[ANETLIST_PRIMITIVE_XORCY_LI];
			if(ep != NULL)
				r = get_driven(ep->inst->outputs[ep->pin],
					&anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY],
					ANETLIST_PRIMITIVE_MUXCY_S);
			else
				r = NULL;
			if(r == NULL)
				return cce; /* special case: chain is one XORCY alone */
			else
				cce = r;
		} else
			cce = r;
	}
	while(1) {
		r = cce->inputs[ANETLIST_PRIMITIVE_MUXCY_CI]->inst;
		if(r->e != &anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY])
			return cce;
		else
			cce = r;
	}
}

static int carrychain_measure_height(struct anetlist_instance *cce)
{
	int height;
	struct anetlist_instance *r;
	
	if(cce->e == &anetlist_primitives[ANETLIST_PRIMITIVE_XORCY])
		return 1; /* special case: chain is one XORCY alone */
	assert(cce->e == &anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY]);

	height = 1;
	while(1) {
		r = get_driven(cce->outputs[ANETLIST_PRIMITIVE_MUXCY_O],
			&anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY],
			ANETLIST_PRIMITIVE_MUXCY_CI);
		if(r == NULL) {
			r = get_driven(cce->outputs[ANETLIST_PRIMITIVE_MUXCY_O],
				&anetlist_primitives[ANETLIST_PRIMITIVE_XORCY],
				ANETLIST_PRIMITIVE_XORCY_CI);
			if(r != NULL)
				height++; /* special case: last MUXCY of the chain has been pruned */
			return height;
		}
		height++;
		cce = r;
	}
}

static void carrychain_enumerate(struct anetlist_instance *cce, struct anetlist_instance **muxcys, struct anetlist_instance **xorcys)
{
	int i;
	struct anetlist_instance *r;
	struct anetlist_endpoint *ep;
	
	if(cce->e == &anetlist_primitives[ANETLIST_PRIMITIVE_XORCY]) {
		/* special case: chain is one XORCY alone */
		muxcys[0] = NULL;
		xorcys[0] = cce;
		return;
	}
	assert(cce->e == &anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY]);
	
	i = 0;
	while(1) {
		muxcys[i] = cce;
		ep = cce->inputs[ANETLIST_PRIMITIVE_MUXCY_S];
		if(ep != NULL)
			xorcys[i] = get_driven(ep->inst->outputs[ep->pin],
				&anetlist_primitives[ANETLIST_PRIMITIVE_XORCY],
				ANETLIST_PRIMITIVE_XORCY_LI);
		else
			xorcys[i] = NULL;
		r = get_driven(cce->outputs[ANETLIST_PRIMITIVE_MUXCY_O],
			&anetlist_primitives[ANETLIST_PRIMITIVE_MUXCY],
			ANETLIST_PRIMITIVE_MUXCY_CI);
		if(r == NULL) {
			r = get_driven(cce->outputs[ANETLIST_PRIMITIVE_MUXCY_O],
				&anetlist_primitives[ANETLIST_PRIMITIVE_XORCY],
				ANETLIST_PRIMITIVE_XORCY_CI);
			if(r != NULL) {
				/* special case: last MUXCY of the chain has been pruned */
				i++;
				muxcys[i] = NULL;
				xorcys[i] = r;
			}
			return;
		}
		i++;
		cce = r;
	}
}

static void transform_carrychain(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *bottom;
	int height, n_carry4;
	struct anetlist_instance **muxcys;
	struct anetlist_instance **xorcys;
	struct anetlist_instance **carry4s;
	int i, i_carry4, c4_offset;
	struct imap *im;
	
	bottom = carrychain_get_bottom(inst);
	height = carrychain_measure_height(bottom);
	printf("packing carry chain from instance %s with %d stages [seed=%s:%s]\n", bottom->uid, height, inst->uid, inst->e->name);
	n_carry4 = (height+3)/4;
	muxcys = alloc_size(height*sizeof(struct anetlist_instance *));
	xorcys = alloc_size(height*sizeof(struct anetlist_instance *));
	carry4s = alloc_size(n_carry4*sizeof(struct anetlist_instance *));
	carrychain_enumerate(bottom, muxcys, xorcys);
	for(i=0;i<n_carry4;i++)
		carry4s[i] = anetlist_instantiate(dst,
			muxcys[4*i] == NULL ? xorcys[4*i]->uid : muxcys[4*i]->uid,
			&anetlist_bels[ANETLIST_BEL_CARRY4]);
	
	for(i=0;i<height;i++) {
		i_carry4 = i/4;
		c4_offset = i%4;
		if(muxcys[i] != NULL) {
			printf("packing MUXCY%d %s\n", i, muxcys[i]->uid);
			im = create_imap(muxcys[i]);
			im->inst = carry4s[i_carry4];
			im->pins[ANETLIST_PRIMITIVE_MUXCY_DI] = 2*c4_offset; /* DIx */
			im->pins[ANETLIST_PRIMITIVE_MUXCY_S] = 2*c4_offset+1; /* Sx */
			if(c4_offset == 0) {
				if(i == 0)
					im->pins[ANETLIST_PRIMITIVE_MUXCY_CI] = ANETLIST_BEL_CARRY4_CYINIT;
				else
					im->pins[ANETLIST_PRIMITIVE_MUXCY_CI] = ANETLIST_BEL_CARRY4_CIN;
			} else
				im->pins[ANETLIST_PRIMITIVE_MUXCY_CI] = -1;
		}
		if(xorcys[i] != NULL) {
			printf("packing XORCY%d %s\n", i, xorcys[i]->uid);
			im = create_imap(xorcys[i]);
			im->inst = carry4s[i_carry4];
			im->pins[ANETLIST_PRIMITIVE_XORCY_LI] = 2*c4_offset+1; /* Sx */
			if(c4_offset == 0) {
				if(i == 0)
					im->pins[ANETLIST_PRIMITIVE_XORCY_CI] = ANETLIST_BEL_CARRY4_CYINIT;
				else
					im->pins[ANETLIST_PRIMITIVE_XORCY_CI] = ANETLIST_BEL_CARRY4_CIN;
			} else
				im->pins[ANETLIST_PRIMITIVE_XORCY_CI] = -1;
		}
	}
	
	free(muxcys);
	free(xorcys);
	free(carry4s);
}

static void transform_fd(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *fdre;
	struct imap *i;
	
	i = create_imap(inst);
	fdre = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_FDRE]);
	i->inst = fdre;
	i->pins[ANETLIST_PRIMITIVE_FD_C] = ANETLIST_BEL_FDRE_C;
	i->pins[ANETLIST_PRIMITIVE_FD_D] = ANETLIST_BEL_FDRE_D;
	match_output(src, dst, fdre, ANETLIST_BEL_FDRE_Q, inst, ANETLIST_PRIMITIVE_FD_Q);
}

static void transform_copy(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *new;
	struct imap *i;
	int j;
	
	i = create_imap(inst);
	new = anetlist_instantiate(dst, inst->uid, inst->e);
	i->inst = new;
	for(j=0;j<inst->e->n_inputs;j++)
		i->pins[j] = j;
	for(j=0;j<inst->e->n_outputs;j++)
		match_output(src, dst, new, j, inst, j);
}

static void transform_inst(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	int pi;
	
	if(inst->user != NULL)
		return;
	if(inst->e->type == ANETLIST_ENTITY_INTERNAL) {
		pi = get_primitive_index(inst->e);
		switch(pi) {
			case ANETLIST_PRIMITIVE_IBUF:
				transform_ibuf(src, dst, inst);
				break;
			case ANETLIST_PRIMITIVE_OBUF:
				transform_obuf(src, dst, inst);
				break;
			case ANETLIST_PRIMITIVE_LUT1:
			case ANETLIST_PRIMITIVE_LUT2:
			case ANETLIST_PRIMITIVE_LUT3:
			case ANETLIST_PRIMITIVE_LUT4:
			case ANETLIST_PRIMITIVE_LUT5:
			case ANETLIST_PRIMITIVE_LUT6:
				transform_lut(src, dst, inst);
				break;
			case ANETLIST_PRIMITIVE_BUFGP:
				transform_bufgp(src, dst, inst);
				break;
			case ANETLIST_PRIMITIVE_MUXCY:
			case ANETLIST_PRIMITIVE_XORCY:
				transform_carrychain(src, dst, inst);
				break;
			case ANETLIST_PRIMITIVE_FD:
				transform_fd(src, dst, inst);
				break;
//			case ANETLIST_PRIMITIVE_FDE:
//				break;
			default:
				if(inst->e->bel)
					transform_copy(src, dst, inst);
				else {
					fprintf(stderr, "Unhandled primitive: %s\n", inst->e->name);
					exit(EXIT_FAILURE);
				}
				break;
		}
		/* an input map must always have been created */
		assert(inst->user != NULL);
	}
}

void transform(struct anetlist *src, struct anetlist *dst)
{
	struct anetlist_instance *inst;
	struct imap *i;
	
	anetlist_set_module_name(dst, src->module_name);
	anetlist_set_part_name(dst, src->part_name);
	
	inst = src->head;
	while(inst != NULL) {
		transform_inst(src, dst, inst);
		inst = inst->next;
	}
	
	/* clean up the input maps */
	inst = src->head;
	while(inst != NULL) {
		i = inst->user;
		if(i != NULL) {
			free(i->pins);
			free(i);
			inst->user = NULL;
		}
		inst = inst->next;
	}
}
