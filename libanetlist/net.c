#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <anetlist/net.h>

struct anetlist *anetlist_new()
{
	struct anetlist *a;
	
	a = alloc_type(struct anetlist);
	a->module_name = NULL;
	a->part_name = NULL;
	a->head = NULL;
	return a;
}

void anetlist_free_endpoint_array(struct anetlist_endpoint **array, int n)
{
	int i;
	struct anetlist_endpoint *ep1, *ep2;
	
	for(i=0;i<n;i++) {
		ep1 = array[i];
		while(ep1 != NULL) {
			ep2 = ep1->next;
			free(ep1);
			ep1 = ep2;
		}
	}
	free(array);
}

static void free_instance(struct anetlist_instance *inst)
{
	int i;
	
	free(inst->uid);
	for(i=0;i<inst->e->n_attributes;i++)
		free(inst->attributes[i]);
	free(inst->attributes);
	anetlist_free_endpoint_array(inst->inputs, inst->e->n_inputs);
	anetlist_free_endpoint_array(inst->outputs, inst->e->n_outputs);
	free(inst);
}

void anetlist_free(struct anetlist *a)
{
	struct anetlist_instance *i1, *i2;
	
	i1 = a->head;
	while(i1 != NULL) {
		i2 = i1->next;
		free_instance(i1);
		i1 = i2;
	}
	free(a->module_name);
	free(a->part_name);
	free(a);
}

void anetlist_remove_instance(struct anetlist *a, struct anetlist_instance *inst)
{
	struct anetlist_instance *prev;
	
	if(inst == a->head)
		a->head = a->head->next;
	else {
		prev = a->head;
		while(prev->next != inst)
			prev = prev->next;
		prev->next = inst->next;
	}
	
	free_instance(inst);
}

void anetlist_disconnect_instance(struct anetlist_instance *inst)
{
	int i;
	struct anetlist_endpoint *ep, *ep2, *sep, *pep;
	
	/* remove sinks this instance outputs to */
	for(i=0;i<inst->e->n_outputs;i++) {
		ep = inst->outputs[i];
		while(ep != NULL) {
			free(ep->inst->inputs[ep->pin]);
			ep->inst->inputs[ep->pin] = NULL;
			ep = ep->next;
		}
	}
	
	/* remove sources this instance sinks */
	for(i=0;i<inst->e->n_inputs;i++) {
		ep = inst->inputs[i];
		if(ep != NULL) {
			sep = ep->inst->outputs[ep->pin];
			pep = NULL;
			while((sep->inst != inst) || (sep->pin != i)) {
				sep = sep->next;
				pep = sep;
			}
			if(sep == ep->inst->outputs[ep->pin]) {
				ep2 = sep->next;
				free(sep);
				ep->inst->outputs[ep->pin] = ep2;
			} else {
				pep->next = sep->next;
				free(sep);
			}
		}
	}
}

void anetlist_set_module_name(struct anetlist *a, const char *name)
{
	free(a->module_name);
	if(name == NULL)
		a->module_name = NULL;
	else
		a->module_name = stralloc(name);
}

void anetlist_set_part_name(struct anetlist *a, const char *name)
{
	free(a->part_name);
	if(name == NULL)
		a->part_name = NULL;
	else
		a->part_name = stralloc(name);
}

void anetlist_init_instance_fields(struct anetlist_entity *e, char ***attributes, struct anetlist_endpoint ***inputs, struct anetlist_endpoint ***outputs)
{
	int i;
	
	if(e->n_attributes == 0)
		*attributes = NULL;
	else {
		*attributes = alloc_size(e->n_attributes*sizeof(char *));
		for(i=0;i<e->n_attributes;i++)
			(*attributes)[i] = stralloc(e->default_attributes[i]);
	}
	if(e->n_inputs == 0)
		*inputs = NULL;
	else
		*inputs = alloc_size0(e->n_inputs*sizeof(struct anetlist_endpoint *));
	if(e->n_outputs == 0)
		*outputs = NULL;
	else
		*outputs = alloc_size0(e->n_outputs*sizeof(struct anetlist_endpoint *));
}

struct anetlist_instance *anetlist_instantiate(struct anetlist *a, const char *uid, struct anetlist_entity *e)
{
	struct anetlist_instance *inst;
	
	inst = alloc_type(struct anetlist_instance);
	inst->uid = stralloc(uid);
	inst->e = e;
	anetlist_init_instance_fields(e, &inst->attributes, &inst->inputs, &inst->outputs);
	inst->next = a->head;
	a->head = inst;

	return inst;
}

void anetlist_set_attribute(struct anetlist_instance *inst, int attr, const char *value)
{
	free(inst->attributes[attr]);
	inst->attributes[attr] = stralloc(value);
}

void anetlist_connect(struct anetlist_instance *start, int output, struct anetlist_instance *end, int input)
{
	struct anetlist_endpoint *ep;
	
	ep = alloc_type(struct anetlist_endpoint);
	ep->inst = start;
	ep->pin = output;
	if(end->inputs[input] != NULL) {
		fprintf(stderr, "Two different outputs driving the same input\n");
		exit(EXIT_FAILURE);
	}
	ep->next = NULL;
	end->inputs[input] = ep;
	
	ep = alloc_type(struct anetlist_endpoint);
	ep->inst = end;
	ep->pin = input;
	ep->next = start->outputs[output];
	start->outputs[output] = ep;
}

struct anetlist_instance *anetlist_find(struct anetlist *a, const char *uid)
{
	struct anetlist_instance *inst;
	
	inst = a->head;
	while(inst != NULL) {
		if(strcmp(inst->uid, uid) == 0)
			return inst;
		inst = inst->next;
	}
	return NULL;
}
