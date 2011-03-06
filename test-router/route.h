#ifndef __ROUTE_H
#define __ROUTE_H

#include <chip/db.h>

void print_wire(struct db *db, struct wire *w, struct tile *wanted_tile);

void route(struct db *db, struct wire *start, struct wire *end);

#endif /* __ROUTE_H */
