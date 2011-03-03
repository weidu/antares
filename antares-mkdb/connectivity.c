#include <assert.h>
#include <stdlib.h>
#include <util.h>

#include <chip/db.h>

#include "connectivity.h"

struct conn *conn_create(struct db *db)
{
	struct conn *c;
	
	c = alloc_type(struct conn);
	c->db = db;
	c->whead = NULL;
	return c;
}

static void free_wire(struct c_wire *w)
{
	struct c_wire_branch *b1, *b2;
	struct c_pip *p1, *p2;
	
	b1 = w->bhead;
	while(b1 != NULL) {
		b2 = b1->next;
		free(b1);
		b1 = b2;
	}
	
	p1 = w->phead;
	while(p1 != NULL) {
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
	
	free(w);
}

void conn_free(struct conn *c)
{
	struct c_wire *w1, *w2;
	
	w1 = c->whead;
	while(w1 != NULL) {
		w2 = w1->next;
		free_wire(w1);
		w1 = w2;
	}
	
	free(c);
}

struct c_wire *conn_get_wire_tile(struct conn *c, struct tile *tile, const char *name)
{
	int iname;
	struct c_wire *w;
	struct c_wire_branch *b;
	
	iname = db_resolve_tile_wire(&c->db->tile_types[tile->type], name);
	
	/* Check for an already existing wire */
	w = c->whead;
	while(w != NULL) {
		b = w->bhead;
		while(b != NULL) {
			if((b->tile == tile) && (b->name == iname))
				return w;
			b = b->next;
		}
		w = w->next;
	}
	
	/* New wire - create it */
	b = alloc_type(struct c_wire_branch);
	b->tile = tile;
	b->name = iname;
	b->next = NULL;
	w = alloc_type(struct c_wire);
	w->bhead = b;
	w->phead = NULL;
	w->joined = NULL;
	w->next = NULL;
	return w;
}

struct c_wire *conn_follow(struct c_wire *w)
{
	while(w->joined != NULL)
		w = w->joined;
	return w;
}

void conn_join_wires(struct c_wire *resulting, struct c_wire *merged)
{
	struct c_wire_branch *blast;
	struct c_pip *plast;
	
	resulting = conn_follow(resulting);
	merged = conn_follow(merged);
	
	if(resulting->bhead == NULL)
		resulting->bhead = merged->bhead;
	else {
		blast = resulting->bhead;
		while(blast->next != NULL)
			blast = blast->next;
		blast->next = merged->bhead;
	}
	
	if(resulting->phead == NULL)
		resulting->phead = merged->phead;
	else {
		plast = resulting->phead;
		while(plast->next != NULL)
			plast = plast->next;
		plast->next = merged->phead;
	}
	
	merged->bhead = NULL;
	merged->phead = NULL;
	merged->joined = resulting;
}

void conn_add_pip(struct tile *tile, int bidir, struct c_wire *w1, struct c_wire *w2)
{
	struct c_pip *p;
	
	p = alloc_type(struct c_pip);
	p->tile = tile;
	p->bidir = bidir;
	p->endpoint = w2;
	p->next = w1->phead;
	w1->phead = p;
}
