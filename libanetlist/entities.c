#include <string.h>

#include <anetlist/entities.h>
#include <anetlist/primitives.h>
#include <anetlist/bels.h>
#include <anetlist/sites.h>

#define FIND_ENTITY(array) \
	int i; \
	\
	for(i=0;i<sizeof(array)/sizeof(array[0]);i++) \
		if(strcmp(array[i].name, name) == 0) \
			return &array[i]; \
	return NULL;

struct anetlist_entity *entity_find_primitive(const char *name)
{
	FIND_ENTITY(anetlist_primitives)
}

struct anetlist_entity *entity_find_bel(const char *name)
{
	FIND_ENTITY(anetlist_bels)
}

struct anetlist_entity *entity_find_site(const char *name)
{
	FIND_ENTITY(anetlist_sites)
}

#undef FIND_ENTITY

int entity_find_attr(struct anetlist_entity *e, const char *attr)
{
	int i;
	
	for(i=0;i<e->n_attributes;i++)
		if(strcmp(e->attribute_names[i], attr) == 0)
			return i;
	return -1;
}

int entity_find_pin(struct anetlist_entity *e, int output, const char *name)
{
	int i;
	
	if(output) {
		if(e->type == ANETLIST_ENTITY_PORT_IN)
			return 0;
		for(i=0;i<e->n_outputs;i++)
			if(strcmp(e->output_names[i], name) == 0)
				return i;
	} else {
		if(e->type == ANETLIST_ENTITY_PORT_OUT)
			return 0;
		for(i=0;i<e->n_inputs;i++)
			if(strcmp(e->input_names[i], name) == 0)
				return i;
	}
	return -1;
}

static char *port_pin_names[] = {"P"};

struct anetlist_entity entity_input_port = {
	.type = ANETLIST_ENTITY_PORT_IN,
	.name = "INPUT_PORT",
	.n_attributes = 0,
	.attribute_names = NULL,
	.default_attributes = NULL,
	.n_inputs = 0,
	.input_names = NULL,
	.n_outputs = 1,
	.output_names = port_pin_names
};

struct anetlist_entity entity_output_port = {
	.type = ANETLIST_ENTITY_PORT_OUT,
	.name = "OUTPUT_PORT",
	.n_attributes = 0,
	.attribute_names = NULL,
	.default_attributes = NULL,
	.n_inputs = 1,
	.input_names = port_pin_names,
	.n_outputs = 0,
	.output_names = NULL
};
