#ifndef __CONNECTIVITY_H
#define __CONNECTIVITY_H

#include <chip/db.h>

struct c_wire {
	struct c_wire *next;
};

struct conn {
	struct db *db;
	struct c_wire *whead;
};

struct c_wire *conn_get_wire_tile(struct conn *c, struct tile *tile, const char *name);

#endif /* __CONNECTIVITY_H */
