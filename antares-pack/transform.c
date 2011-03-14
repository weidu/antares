#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <anetlist/entities.h>
#include <anetlist/net.h>
#include <anetlist/primitives.h>
#include <anetlist/bels.h>

static int is_external(struct anetlist_instance **instances, int n_instances, struct anetlist_instance *inst)
{
	int i;
	
	for(i=0;i<n_instances;i++)
		if(instances[i] == inst)
			return 0;
	return 1;
}

static int lookup_pin_to(struct anetlist_endpoint **eps, int n_eps, struct anetlist_instance *inst, int pin)
{
	int i;
	struct anetlist_endpoint *ep;
	
	printf("looking for pin %d of %p (%s)\n", pin, inst, inst->e->name);
	for(i=0;i<n_eps;i++) {
		ep = eps[i];
		while(ep != NULL) {
			printf("hit pin %d of %p (%s)\n", ep->pin, ep->inst, ep->inst->e->name);
			if((ep->inst == inst) && (ep->pin == pin))
				return i;
			ep = ep->next;
		}
	}
	return -1;
}

static void replace(struct anetlist *a, struct anetlist_instance **instances, int n_instances, struct anetlist_entity *te, char **tattributes, struct anetlist_endpoint **tinputs, struct anetlist_endpoint **toutputs)
{
	int i, j;
	struct anetlist_endpoint *ep, *extep;
	
	/* update cross references */
	for(i=0;i<n_instances;i++) {
		/* outputs of the replaced block: update external inputs they are connected to */
		for(j=0;j<instances[i]->e->n_outputs;j++) {
			ep = instances[i]->outputs[j];
			while(ep != NULL) {
				if(is_external(instances, n_instances, ep->inst)) {
					extep = ep->inst->inputs[ep->pin];
					assert((extep != NULL) && (extep->next == NULL));
					extep->inst = instances[0];
					extep->pin = lookup_pin_to(toutputs, te->n_outputs, ep->inst, ep->pin);
					assert(extep->pin != -1);
				}
				ep = ep->next;
			}
		}
		
		/* inputs of the replaced block: update external outputs they are connected to */
		for(j=0;j<instances[i]->e->n_inputs;j++) {
			ep = instances[i]->inputs[j];
			if(ep != NULL) {
				if(is_external(instances, n_instances, ep->inst)) {
					extep = ep->inst->outputs[ep->pin];
					while((extep->inst != instances[i]) || (extep->pin != j))
						extep = extep->next;
					extep->inst = instances[0];
					extep->pin = lookup_pin_to(tinputs, te->n_inputs, ep->inst, ep->pin);
					assert(extep->pin != -1);
				}
			}
		}
	}
	
	/* remove absorbed instances */
	for(i=1;i<n_instances;i++)
		anetlist_remove_instance(a, instances[i]);
	
	/* do the replacement proper */
	for(i=0;i<instances[0]->e->n_attributes;i++)
		free(instances[0]->attributes[i]);
	free(instances[0]->attributes);
	instances[0]->attributes = tattributes;
	anetlist_free_endpoint_array(instances[0]->inputs, instances[0]->e->n_inputs);
	instances[0]->inputs = tinputs;
	anetlist_free_endpoint_array(instances[0]->outputs, instances[0]->e->n_outputs);
	instances[0]->outputs = toutputs;
	instances[0]->e = te;
}

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

static struct anetlist_endpoint *endpoints_dup(struct anetlist_endpoint *ep)
{
	struct anetlist_endpoint *r;
	struct anetlist_endpoint *n;
	
	r = NULL;
	while(ep != NULL) {
		n = alloc_type(struct anetlist_endpoint);
		n->inst = ep->inst;
		n->pin = ep->pin;
		n->next = r;
		r = n;
		ep = ep->next;
	}
	
	return r;
}

static void transform_ibuf(struct anetlist *a, struct anetlist_instance *ibuf)
{
	struct anetlist_entity *te;
	char **tattributes;
	struct anetlist_endpoint **tinputs;
	struct anetlist_endpoint **toutputs;
	struct anetlist_instance *instances[2];
	
	te = &anetlist_bels[ANETLIST_BEL_IOBM];
	anetlist_init_instance_fields(te, &tattributes, &tinputs, &toutputs);
	// TODO: how to set the IOB in pure input mode?
	toutputs[ANETLIST_BEL_IOBM_I] = endpoints_dup(ibuf->outputs[ANETLIST_PRIMITIVE_IBUF_O]);
	instances[0] = ibuf;
	instances[1] = ibuf->inputs[ANETLIST_PRIMITIVE_IBUF_I]->inst; /* remove the input port */
	replace(a, instances, 2, te, tattributes, tinputs, toutputs);
}

static void transform_obuf(struct anetlist *a, struct anetlist_instance *obuf)
{
	struct anetlist_entity *te;
	char **tattributes;
	struct anetlist_endpoint **tinputs;
	struct anetlist_endpoint **toutputs;
	struct anetlist_instance *instances[2];
	
	te = &anetlist_bels[ANETLIST_BEL_IOBM];
	anetlist_init_instance_fields(te, &tattributes, &tinputs, &toutputs);
	// TODO: how to set the IOB in pure output mode?
	tinputs[ANETLIST_BEL_IOBM_O] = endpoints_dup(obuf->inputs[ANETLIST_PRIMITIVE_OBUF_I]);
	instances[0] = obuf;
	instances[1] = obuf->outputs[ANETLIST_PRIMITIVE_OBUF_O]->inst; /* remove the output port */
	replace(a, instances, 2, te, tattributes, tinputs, toutputs);
}

void transform(struct anetlist *a)
{
	struct anetlist_instance *inst;
	int pi;
	
	inst = a->head;
	while(inst != NULL) {
		switch(inst->e->type) {
			case ANETLIST_ENTITY_INTERNAL:
				pi = get_primitive_index(inst->e);
				switch(pi) {
					case ANETLIST_PRIMITIVE_IBUF:
						transform_ibuf(a, inst);
						break;
					case ANETLIST_PRIMITIVE_OBUF:
						transform_obuf(a, inst);
						break;
					default:
						fprintf(stderr, "Unhandled primitive: %s\n", inst->e->name);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			case ANETLIST_ENTITY_PORT_OUT:
			case ANETLIST_ENTITY_PORT_IN:
				/* nothing to do: those get removed as part of I/O buffer transformation */
				break;
			default:
				assert(0);
				break;
		}
		inst = inst->next;
	}
}
