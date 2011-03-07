#ifndef __ANETLIST_NET_H
#define __ANETLIST_NET_H

enum {
	ANETLIST_ENTITY_INTERNAL,
	ANETLIST_ENTITY_PORT_OUT,
	ANETLIST_ENTITY_PORT_IN
};

struct anetlist_entity {
	int type;			/* < internal or I/O port */
	char *name;			/* < name of the entity */
	int attribute_count;		/* < number of attributes */
	char **attribute_names;		/* < names of attributes */
	char **default_attributes;	/* < default values of attributes */
	int inputs;			/* < number of inputs */
	char **input_names;		/* < names of the inputs */
	int outputs;			/* < number of outputs */
	char **output_names;		/* < names of the outputs */
};

#endif /* __ANETLIST_NET_H */
