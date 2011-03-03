#ifndef __CONNECTIVITY_H
#define __CONNECTIVITY_H

#include <chip/db.h>

struct c_wire_branch {
	struct tile *tile;
	int name;
	struct c_wire_branch *next;
};

struct c_wire;

struct c_pip {
	struct tile *tile;
	struct c_wire *endpoint;
	int bidir;
	struct c_pip *next;
};

struct c_wire {
	struct c_wire_branch *bhead;
	struct c_pip *phead;
	struct c_wire *joined;
	struct c_wire *next;
};

struct conn {
	struct db *db;
	struct c_wire *whead;
};

struct conn *conn_create(struct db *db);
void conn_free(struct conn *c);

struct c_wire *conn_get_wire_tile(struct conn *c, struct tile *tile, const char *name);
struct c_wire *conn_follow(struct c_wire *w);
void conn_join_wires(struct c_wire *resulting, struct c_wire *merged);
void conn_add_pip(struct tile *tile, int bidir, struct c_wire *w1, struct c_wire *w2);

#endif /* __CONNECTIVITY_H */
