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

static void transform_inst(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst);

static void match_output(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *new, int newoutput, struct anetlist_instance *inst, int instoutput)
{
	struct anetlist_endpoint *ep;
	struct imap *i;
	
	ep = inst->outputs[instoutput];
	while(ep != NULL) {
		transform_inst(src, dst, ep->inst);
		i = ep->inst->user;
		anetlist_connect(new, newoutput, i->inst, i->pins[ep->pin]);
		ep = ep->next;
	}
}

/* TODO: how to set the IOBs in simple input/output mode? */
static void transform_ibuf(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *iobm;
	struct imap *i;
	
	i = inst->user;
	iobm = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_IOBM]);
	i->inst = iobm;
	match_output(src, dst, iobm, ANETLIST_BEL_IOBM_I, inst, ANETLIST_PRIMITIVE_IBUF_O);
}

static void transform_obuf(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *iobm;
	struct imap *i;
	
	i = inst->user;
	iobm = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_IOBM]);
	i->inst = iobm;
	i->pins[ANETLIST_PRIMITIVE_OBUF_I] = ANETLIST_BEL_IOBM_O;
}

static void transform_lut(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *lut6_2;
	struct imap *i;
	int j;
	
	i = inst->user;
	lut6_2 = anetlist_instantiate(dst, inst->uid, &anetlist_bels[ANETLIST_BEL_LUT6_2]);
	i->inst = lut6_2;
	for(j=0;j<inst->e->n_inputs;j++)
		i->pins[j] = j;
	strcpy(lut6_2->attributes[0]+16-strlen(inst->attributes[0]), inst->attributes[0]);
	match_output(src, dst, lut6_2, ANETLIST_BEL_LUT6_2_O6, inst, 0);
}

static void transform_fd(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	struct anetlist_instance *fdre;
	struct imap *i;
	
	i = inst->user;
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
	
	i = inst->user;
	new = anetlist_instantiate(dst, inst->uid, inst->e);
	i->inst = new;
	for(j=0;j<inst->e->n_inputs;j++)
		i->pins[j] = j;
	for(j=0;j<inst->e->n_outputs;j++)
		match_output(src, dst, new, j, inst, j);
}

static struct imap *create_imap(int n_inputs)
{
	struct imap *i;
	
	i = alloc_type(struct imap);
	i->pins = alloc_size(n_inputs*sizeof(int));
	return i;
}

static void transform_inst(struct anetlist *src, struct anetlist *dst, struct anetlist_instance *inst)
{
	int pi;
	
	if(inst->user != NULL)
		return;
	inst->user = create_imap(inst->e->n_inputs);
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
				break;
			case ANETLIST_PRIMITIVE_MUXCY:
			case ANETLIST_PRIMITIVE_XORCY:
				break;
			case ANETLIST_PRIMITIVE_FD:
				transform_fd(src, dst, inst);
				break;
			case ANETLIST_PRIMITIVE_FDE:
				break;
			default:
				if(inst->e->bel)
					transform_copy(src, dst, inst);
				else {
					fprintf(stderr, "Unhandled primitive: %s\n", inst->e->name);
					exit(EXIT_FAILURE);
				}
				break;
		}
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
