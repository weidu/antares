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
void conn_join_wires(struct c_wire *resulting, struct c_wire *merged);
void conn_add_pip(struct conn *c, struct tile *tile, int bidir, struct c_wire *w2, struct c_wire *w1);

#endif /* __CONNECTIVITY_H */
