#include <stdlib.h>
#include <util.h>

#include <anetlist/net.h>

struct anetlist *anetlist_create()
{
	struct anetlist *a;
	
	a = alloc_type(struct anetlist);
	a->head = NULL;
	return a;
}

static void free_instance(struct anetlist_instance *inst)
{
	int i;
	struct anetlist_endpoint *ep1, *ep2;
	
	free(inst->uid);
	for(i=0;i<inst->e->n_attributes;i++)
		free(inst->attributes[i]);
	free(inst->attributes);
	for(i=0;i<inst->e->n_outputs;i++) {
		ep1 = inst->outputs[i];
		while(ep1 != NULL) {
			ep2 = ep1->next;
			free(ep1);
			ep1 = ep2;
		}
	}
	free(inst->outputs);
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
	free(a);
}

struct anetlist_instance *anetlist_instantiate(struct anetlist *a, const char *uid, struct anetlist_entity *e)
{
	struct anetlist_instance *inst;
	int i;
	
	inst = alloc_type(struct anetlist_instance);
	inst->uid = stralloc(uid);
	inst->e = e;
	inst->attributes = alloc_size(e->n_attributes*sizeof(char *));
	for(i=0;i<e->n_attributes;i++)
		inst->attributes[i] = stralloc(e->default_attributes[i]);
	inst->outputs = alloc_size0(e->n_outputs*sizeof(struct anetlist_endpoint *));
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
	ep->inst = end;
	ep->input = input;
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
