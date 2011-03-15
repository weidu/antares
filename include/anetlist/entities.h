#ifndef __ANETLIST_ENTITIES_H
#define __ANETLIST_ENTITIES_H

enum {
	ANETLIST_ENTITY_INTERNAL,
	ANETLIST_ENTITY_PORT_OUT,
	ANETLIST_ENTITY_PORT_IN
};

struct anetlist_entity {
	int type;				/* < internal or I/O port */
	int bel;				/* < can this entity be a BEL? */
	char *name;				/* < name of the entity */
	int n_attributes;			/* < number of attributes */
	char **attribute_names;			/* < names of attributes */
	char **default_attributes;		/* < default values of attributes */
	int n_inputs;				/* < number of inputs */
	char **input_names;			/* < names of the inputs */
	int n_outputs;				/* < number of outputs */
	char **output_names;			/* < names of the outputs */
};

extern struct anetlist_entity entity_input_port;
extern struct anetlist_entity entity_output_port;

struct anetlist_entity *entity_find_primitive(const char *name);
struct anetlist_entity *entity_find_bel(const char *name);
struct anetlist_entity *entity_find_site(const char *name);

int entity_find_attr(struct anetlist_entity *e, const char *attr);
int entity_find_pin(struct anetlist_entity *e, int output, const char *name);

#endif /* __ANETLIST_ENTITIES_H */
