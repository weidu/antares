#ifndef __ANETLIST_NET_H
#define __ANETLIST_NET_H

#include <anetlist/entities.h>

struct anetlist_endpoint;

struct anetlist_instance {
	char *uid;				/* < unique identifier */
	struct anetlist_entity *e;		/* < what entity we are an instance of */
	char **attributes;			/* < attributes of this instance */
	struct anetlist_endpoint **outputs;	/* < array of what each output is connected to */
	struct anetlist_instance *next;		/* < next instance in this manager */
};

struct anetlist_endpoint {
	struct anetlist_instance *inst;		/* < instance at the endpoint */
	int input;				/* < index of the input in the instance */
	struct anetlist_endpoint *next;		/* < next endpoint on this output */
};

struct anetlist {
	struct anetlist_instance *head;
};

struct anetlist *anetlist_create();
void anetlist_free(struct anetlist *a);

struct anetlist_instance *anetlist_instantiate(struct anetlist *a, const char *uid, struct anetlist_entity *e);
void anetlist_set_attribute(struct anetlist_instance *inst, int attr, const char *value);
void anetlist_connect(struct anetlist_instance *start, int output, struct anetlist_instance *end, int input);

struct anetlist_instance *anetlist_find(struct anetlist *a, const char *uid);

#endif /* __ANETLIST_NET_H */
