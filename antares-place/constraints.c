#include <stdlib.h>
#include <util.h>

#include <anetlist/net.h>
#include <anetlist/bels.h>

#include "constraints.h"

void constraints_init(struct anetlist *a)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;
		
	inst = a->head;
	while(inst != NULL) {
		constraint = alloc_type(struct constraint);
		constraint->group = NULL;
		constraint->chain_above = NULL;
		constraint->chain_below = NULL;
		constraint->lock = NULL;
		constraint->current = NULL;
		inst->user = constraint;
		inst = inst->next;
	}
}

static void group_append(struct constraint_group *head, struct constraint_group *added)
{
	while(head->next != NULL)
		head = head->next;
	head->next = added;
}

void constraints_same_slice(struct anetlist_instance *i1, struct anetlist_instance *i2)
{
	struct constraint *c1, *c2;
	struct constraint_group *g1, *g2;
	
	c1 = i1->user;
	c2 = i2->user;
	if((c1->group == NULL) && (c2->group == NULL)) {
		g1 = alloc_type(struct constraint_group);
		g2 = alloc_type(struct constraint_group);
		g1->inst = i1;
		g1->next = g2;
		g2->inst = i2;
		g2->next = NULL;
		c1->group = g1;
		c2->group = g1;
	} else if((c1->group != NULL) && (c2->group == NULL)) {
		c2->group = c1->group;
		g1 = alloc_type(struct constraint_group);
		g1->inst = i2;
		g1->next = NULL;
		group_append(c1->group, g1);
	} else if((c1->group == NULL) && (c2->group != NULL)) {
		c1->group = c2->group;
		g1 = alloc_type(struct constraint_group);
		g1->inst = i1;
		g1->next = NULL;
		group_append(c2->group, g1);
	} else {
		/* both non-NULL */
		group_append(c1->group, c2->group);
		/* update list heads in instance 2 group */
		g1 = c2->group;
		while(g1 != NULL) {
			c2 = g1->inst->user;
			c2->group = c1->group;
			g1 = g1->next;
		}
	}
}

void constraints_lock(struct anetlist_instance *inst, struct resmgr_bel *bel)
{
	struct constraint *constraint;
	
	constraint = inst->user;
	constraint->lock = bel;
}

static void propagate_lock(struct anetlist *a)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;

	inst = a->head;
	while(inst != NULL) {
		constraint = inst->user;
		if(constraint->lock != NULL) {
		}
		inst = inst->next;
	}
}

static void same_slice_driving_lut(struct anetlist_instance *inst, struct anetlist_endpoint *ep)
{
	if((ep != NULL) 
	  && (ep->inst->e == &anetlist_bels[ANETLIST_BEL_LUT6_2])
	  && (ep->pin == ANETLIST_BEL_LUT6_2_O6))
		constraints_same_slice(inst, ep->inst);
}

static void infer_rel_fdre(struct anetlist_instance *inst)
{
	struct anetlist_endpoint *ep;
	
	ep = inst->inputs[ANETLIST_BEL_FDRE_D];
	if((ep != NULL) && (
	    (ep->inst->e == &anetlist_bels[ANETLIST_BEL_LUT6_2])
	  ||(ep->inst->e == &anetlist_bels[ANETLIST_BEL_MUXF7])
	  ||(ep->inst->e == &anetlist_bels[ANETLIST_BEL_MUXF8])
	  ||(ep->inst->e == &anetlist_bels[ANETLIST_BEL_CARRY4])
	))
		constraints_same_slice(inst, ep->inst);
}

static void infer_rel_muxf7(struct anetlist_instance *inst)
{
	if(inst->inputs[ANETLIST_BEL_MUXF7_I0] != NULL)
		constraints_same_slice(inst, inst->inputs[ANETLIST_BEL_MUXF7_I0]->inst);
	if(inst->inputs[ANETLIST_BEL_MUXF7_I1] != NULL)
		constraints_same_slice(inst, inst->inputs[ANETLIST_BEL_MUXF7_I1]->inst);
}

static void infer_rel_muxf8(struct anetlist_instance *inst)
{
	if(inst->inputs[ANETLIST_BEL_MUXF8_I0] != NULL)
		constraints_same_slice(inst, inst->inputs[ANETLIST_BEL_MUXF8_I0]->inst);
	if(inst->inputs[ANETLIST_BEL_MUXF8_I1] != NULL)
		constraints_same_slice(inst, inst->inputs[ANETLIST_BEL_MUXF8_I1]->inst);
}

static void infer_rel_carry4(struct anetlist_instance *inst)
{
	struct constraint *constraint = inst->user;
	struct anetlist_endpoint *ep;
	
	/*
	 * If our CO3 output drives solely the carry in of another CARRY4,
	 * put that one immediately above to enable the use of dedicated
	 * carry chain routing resources.
	 */
	ep = inst->outputs[ANETLIST_BEL_CARRY4_CO3];
	if((ep != NULL) 
	  && (ep->next == NULL)
	  && (ep->inst->e == &anetlist_bels[ANETLIST_BEL_CARRY4])
	  && (ep->pin == ANETLIST_BEL_CARRY4_CIN)
	  && (ep->inst != inst))
		constraint->chain_above = ep->inst;
	/*
	 * Similarly, if our carry in is the only driven pin by the CO3 output
	 * of another CARRY4, place that one immediately below.
	 */
	ep = inst->inputs[ANETLIST_BEL_CARRY4_CIN];
	if((ep != NULL) 
	  && (ep->inst->e == &anetlist_bels[ANETLIST_BEL_CARRY4])
	  && (ep->pin == ANETLIST_BEL_CARRY4_CO3)
	  && (ep->inst->outputs[ANETLIST_BEL_CARRY4_CO3]->next == NULL)
	  && (ep->inst != inst))
		constraint->chain_below = ep->inst;
	/*
	 * Constrain driving LUTs to the same slice.
	 */
	same_slice_driving_lut(inst, inst->inputs[ANETLIST_BEL_CARRY4_S0]);
	same_slice_driving_lut(inst, inst->inputs[ANETLIST_BEL_CARRY4_S1]);
	same_slice_driving_lut(inst, inst->inputs[ANETLIST_BEL_CARRY4_S2]);
	same_slice_driving_lut(inst, inst->inputs[ANETLIST_BEL_CARRY4_S3]);
}

void constraints_infer_rel(struct anetlist *a)
{
	struct anetlist_instance *inst;

	inst = a->head;
	while(inst != NULL) {
		if(inst->e == &anetlist_bels[ANETLIST_BEL_FDRE])
			infer_rel_fdre(inst);
		if(inst->e == &anetlist_bels[ANETLIST_BEL_MUXF7])
			infer_rel_muxf7(inst);
		if(inst->e == &anetlist_bels[ANETLIST_BEL_MUXF8])
			infer_rel_muxf8(inst);
		if(inst->e == &anetlist_bels[ANETLIST_BEL_CARRY4])
			infer_rel_carry4(inst);
		inst = inst->next;
	}
	propagate_lock(a);
}

static void free_group(struct constraint_group *head)
{
	struct constraint_group *g1, *g2;
	struct constraint *constraint;
	
	g1 = head;
	while(g1 != NULL) {
		constraint = g1->inst->user;
		constraint->group = NULL;
		g1 = g1->next;
	}
	
	g1 = head;
	while(g1 != NULL) {
		g2 = g1->next;
		free(g1);
		g1 = g2;
	}
}

void constraints_free(struct anetlist *a)
{
	struct anetlist_instance *inst;
	struct constraint *constraint;

	inst = a->head;
	while(inst != NULL) {
		constraint = inst->user;
		free_group(constraint->group);
		free(constraint);
		inst = inst->next;
	}
}
