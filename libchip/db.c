#include <stdlib.h>
#include <string.h>

#include <util.h>

#include <chip/db.h>

struct db *db_create(int n_tile_types, int n_site_types, int w, int h)
{
	struct db *db;
	
	db = alloc_type(struct db);
	db->chip_ref = NULL;
	db->chip.w = w;
	db->chip.h = h;
	db->chip.tiles = alloc_size0(w*h*sizeof(struct tile));
	db->chip.whead = NULL;
	db->n_tile_types = n_tile_types;
	db->tile_types = alloc_size0(n_tile_types*sizeof(struct tile_type));
	db->n_site_types = n_site_types;
	db->site_types = alloc_size0(n_site_types*sizeof(struct site_type));
	
	return db;
}

static void free_tile_wire(struct tile_wire *tw)
{
	struct pip *p1, *p2;
	
	p1 = tw->phead;
	while(p1 != NULL) {
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}
	free(tw);
}

static void free_wire(struct wire *w)
{
	struct tile_wire *tw1, *tw2;
	
	tw1 = w->whead;
	while(tw1 != NULL) {
		tw2 = tw1->next;
		free_tile_wire(tw1);
		tw1 = tw2;
	}
	free(w);
}

void db_free(struct db *db)
{
	int i, j;
	struct wire *w1, *w2;
	
	free(db->chip_ref);
	for(i=0;i<db->chip.w*db->chip.h;i++)
		free(db->chip.tiles[i].sites);
	free(db->chip.tiles);
	
	w1 = db->chip.whead;
	while(w1 != NULL) {
		w2 = w1->next;
		free_wire(w1);
		w1 = w2;
	}
	
	for(i=0;i<db->n_tile_types;i++) {
		free(db->tile_types[i].name);
		free(db->tile_types[i].sites);
		for(j=0;j<db->tile_types[i].n_tile_wires;j++)
			free(db->tile_types[i].tile_wire_names[j]);
		free(db->tile_types[i].tile_wire_names);
	}
	free(db->tile_types);
	
	for(i=0;i<db->n_site_types;i++) {
		free(db->site_types[i].name);
		for(j=0;j<db->site_types[i].n_inputs;j++)
			free(db->site_types[i].input_pin_names[j]);
		free(db->site_types[i].input_pin_names);
		for(j=0;j<db->site_types[i].n_outputs;j++)
			free(db->site_types[i].output_pin_names[j]);
		free(db->site_types[i].output_pin_names);
	}
	free(db->site_types);

	free(db);
}

