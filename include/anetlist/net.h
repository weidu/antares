#ifndef __ANETLIST_NET_H
#define __ANETLIST_NET_H

#include <anetlist/entities.h>

struct anetlist_endpoint;

struct anetlist_instance {
	char *uid;				/* < unique identifier */
	struct anetlist_entity *e;		/* < what entity we are an instance of */
	char **attributes;			/* < attributes of this instance */
	struct anetlist_endpoint **inputs;	/* < array of what each input is connected to */
	struct anetlist_endpoint **outputs;	/* < array of what each output is connected to */
	void *user;				/* < for application use */
	struct anetlist_instance *next;		/* < next instance in this manager */
};

struct anetlist_endpoint {
	struct anetlist_instance *inst;		/* < instance at the endpoint */
	int pin;				/* < index of the pin in the instance */
	struct anetlist_endpoint *next;		/* < next endpoint on this output */
};

struct anetlist {
	char *module_name;
	char *part_name;
	struct anetlist_instance *head;
};

struct anetlist *anetlist_new();
void anetlist_free_endpoint_array(struct anetlist_endpoint **array, int n);
void anetlist_free(struct anetlist *a);
void anetlist_remove_instance(struct anetlist *a, struct anetlist_instance *inst);
void anetlist_disconnect_instance(struct anetlist_instance *inst);

void anetlist_set_module_name(struct anetlist *a, const char *name);
void anetlist_set_part_name(struct anetlist *a, const char *name);

void anetlist_init_instance_fields(struct anetlist_entity *e, char ***attributes, struct anetlist_endpoint ***inputs, struct anetlist_endpoint ***outputs);
struct anetlist_instance *anetlist_instantiate(struct anetlist *a, const char *uid, struct anetlist_entity *e);
void anetlist_set_attribute(struct anetlist_instance *inst, int attr, const char *value);
void anetlist_connect(struct anetlist_instance *start, int output, struct anetlist_instance *end, int input);

struct anetlist_instance *anetlist_find(struct anetlist *a, const char *uid);

#endif /* __ANETLIST_NET_H */
