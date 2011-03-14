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
	
	for(i=0;i<n_eps;i++) {
		ep = eps[i];
		while(ep != NULL) {
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
					while((extep->inst != ep->inst) || (extep->pin != ep->pin))
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
	instances[0]->e = te;
	for(i=0;i<instances[0]->e->n_attributes;i++)
		free(instances[0]->attributes[i]);
	free(instances[0]->attributes);
	instances[0]->attributes = tattributes;
	anetlist_free_endpoint_array(instances[0]->inputs, instances[0]->e->n_inputs);
	instances[0]->inputs = tinputs;
	anetlist_free_endpoint_array(instances[0]->outputs, instances[0]->e->n_outputs);
	instances[0]->outputs = toutputs;
}

static int get_primitive_index(struct anetlist_entity *e)
{
	int len;
	int index;
	
	len = sizeof(anetlist_bels)/sizeof(anetlist_bels[0]);
	index = e - anetlist_bels;
	if(index >= len)
		index = -1;
	return index;
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
					default:
						fprintf(stderr, "Unhandled primitive: %s\n", inst->e->name);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			case ANETLIST_ENTITY_PORT_OUT:
				break;
			case ANETLIST_ENTITY_PORT_IN:
				break;
			default:
				assert(0);
				break;
		}
		inst = inst->next;
	}
}
