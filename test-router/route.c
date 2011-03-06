#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <chip/db.h>

#include "route.h"

void print_wire(struct db *db, struct wire *w, struct tile *wanted_tile)
{
	int i;
	struct tile *tile;
	
	for(i=0;i<w->n_tile_wires;i++) {
		tile = &db->chip.tiles[w->tile_wires[i].tile];
		if(wanted_tile == NULL)
			printf(" %s_X%dY%d:", db->tile_types[tile->type].name, tile->x, tile->y);
		if((wanted_tile == NULL) || (tile == wanted_tile))
			printf("%s", db->tile_types[tile->type].tile_wire_names[w->tile_wires[i].name]);
	}
}

void print_pip(struct db *db, struct wire *startpoint, struct pip *p)
{
	struct tile *tile;
	
	tile = &db->chip.tiles[p->tile];
	printf("  pip %s_X%dY%d ", db->tile_types[tile->type].name, tile->x, tile->y);
	print_wire(db, startpoint, tile);
	printf(" -> ");
	print_wire(db, &db->chip.wires[p->endpoint], tile);
	printf(" , \n");
}

struct reached_wire {
	struct reached_wire *leading_wire;
	struct pip *leading_pip;
	struct wire *this_wire;
	int marker;
	struct reached_wire *next;
};

struct router {
	struct db *db;
	struct wire *end;
	struct reached_wire *head;
};

static struct router *router_create(struct db *db, struct wire *start, struct wire *end)
{
	struct router *r;
	
	r = alloc_type(struct router);

	r->db = db;
	r->end = end;
	
	r->head = alloc_type(struct reached_wire);
	r->head->leading_wire = NULL;
	r->head->leading_pip = NULL;
	r->head->this_wire = start;
	r->head->marker = 0;
	r->head->next = NULL;
	
	return r;
}

static struct reached_wire *router_reach(struct router *r, struct reached_wire *leading_wire, struct pip *leading_pip, struct wire *w)
{
	struct reached_wire *rw;
	
	if(w->user != NULL)
		return NULL;
	w->user = (void *)1;
	
	rw = alloc_type(struct reached_wire);
	rw->leading_wire = leading_wire;
	rw->leading_pip = leading_pip;
	rw->this_wire = w;
	rw->marker = 0;
	rw->next = r->head;
	r->head = rw;
	return rw;
}

static struct reached_wire *router_iterate(struct router *r)
{
	struct reached_wire *rw, *markerpos;
	struct wire *w;
	int i;
	struct reached_wire *result;
	
	markerpos = r->head;
	rw = r->head;
	while((rw != NULL) && !rw->marker) {
		w = rw->this_wire;
		for(i=0;i<w->n_pips;i++) {
			result = router_reach(r, rw, &w->pips[i], &r->db->chip.wires[w->pips[i].endpoint]);
			if(&r->db->chip.wires[w->pips[i].endpoint] == r->end)
				return result;
		}
		rw = rw->next;
	}
	markerpos->marker = 1;
	return NULL;
}

static void router_backtrace(struct router *r, struct reached_wire *rw)
{
	while(1) {
		if(rw->leading_wire == NULL)
			break;
		print_pip(r->db, rw->leading_wire->this_wire, rw->leading_pip);
		rw = rw->leading_wire;
	}
}

static void router_free(struct router *r)
{
	struct reached_wire *rw1, *rw2;
	
	rw1 = r->head;
	while(rw1 != NULL) {
		rw2 = rw1->next;
		free(rw1);
		rw1 = rw2;
	}
	free(r);
}

void route(struct db *db, struct wire *start, struct wire *end)
{
	struct router *r;
	struct reached_wire *rw;
	int i;
	
	printf("Routing");
	print_wire(db, start, NULL);
	printf(" ->");
	print_wire(db, end, NULL);
	printf("...\n");
	
	if(start == end)
		return;
	r = router_create(db, start, end);
	for(i=0;i<100;i++) {
		rw = router_iterate(r);
		if(rw != NULL) {
			printf("Routing succeeded in %d iterations:\n", i+1);
			router_backtrace(r, rw);
			break;
		}
	}
	router_free(r);
}
