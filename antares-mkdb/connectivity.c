#include <stdlib.h>
#include <util.h>

#include <chip/db.h>

#include "connectivity.h"

struct c_wire *conn_get_wire_tile(struct conn *c, struct tile *tile, const char *name)
{
	return NULL;
}

void conn_join_wires(struct c_wire *resulting, struct c_wire *merged)
{
}

void conn_add_pip(struct conn *c, struct tile *tile, int bidir, struct c_wire *w2, struct c_wire *w1)
{
}
