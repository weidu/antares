#include <assert.h>
#include <stdlib.h>
#include <util.h>

#include <chip/db.h>

#include "connectivity.h"

/* Keep lists in each tile of the wires going through it to increase performance */
struct c_tile_wire_list {
	struct c_wire *w;
	struct c_tile_wire_list *next;
};

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
	int i;
	struct c_tile_wire_list *tw1, *tw2;
	
	w1 = c->whead;
	while(w1 != NULL) {
		w2 = w1->next;
		free_wire(w1);
		w1 = w2;
	}
	
	for(i=0;i<c->db->chip.w*c->db->chip.h;i++) {
		tw1 = c->db->chip.tiles[i].user;
		while(tw1 != NULL) {
			tw2 = tw1->next;
			free(tw1);
			tw1 = tw2;
		}
		c->db->chip.tiles[i].user = NULL;
	}
	
	free(c);
}

struct c_wire *conn_get_wire_tile(struct conn *c, struct tile *tile, const char *name)
{
	int iname;
	struct c_wire *w;
	struct c_wire_branch *b;
	struct c_tile_wire_list *tw;
	
	iname = db_resolve_tile_wire(&c->db->tile_types[tile->type], name);
	
	/* Check for an already existing wire */
	tw = tile->user;
	while(tw != NULL) {
		b = tw->w->bhead;
		while(b != NULL) {
			if((b->tile == tile) && (b->name == iname))
				return tw->w;
			b = b->next;
		}
		tw = tw->next;
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
	w->next = c->whead;
	c->whead = w;
	
	/* Register it into the tile */
	tw = alloc_type(struct c_tile_wire_list);
	tw->w = w;
	tw->next = tile->user;
	tile->user = tw;
	
	return w;
}

struct c_wire *conn_follow(struct c_wire *w)
{
	while(w->joined != NULL)
		w = w->joined;
	return w;
}

void conn_join_wires(struct conn *c, struct c_wire *resulting, struct c_wire *merged)
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

int conn_count_wires(struct conn *c)
{
	int count;
	struct c_wire *w;

	count = 0;
	w = c->whead;
	while(w != NULL) {
		if(w->bhead != NULL)
			count++;
		w = w->next;
	}
	return count;
}
