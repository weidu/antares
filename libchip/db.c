#include <stdlib.h>
#include <stdio.h>
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
	db->chip.n_wires = 0; /* filled later */
	db->chip.wires = NULL;
	db->n_tile_types = n_tile_types;
	db->tile_types = alloc_size0(n_tile_types*sizeof(struct tile_type));
	db->n_site_types = n_site_types;
	db->site_types = alloc_size0(n_site_types*sizeof(struct site_type));
	
	return db;
}

void db_free(struct db *db)
{
	int i, j;
	
	free(db->chip_ref);
	
	/* tiles */
	for(i=0;i<db->chip.w*db->chip.h;i++) {
		for(j=0;j<db->tile_types[db->chip.tiles[i].type].n_sites;j++) {
			free(db->chip.tiles[i].sites[j].name);
			free(db->chip.tiles[i].sites[j].input_wires);
			free(db->chip.tiles[i].sites[j].output_wires);
		}
		free(db->chip.tiles[i].sites);
	}
	free(db->chip.tiles);
	
	/* wires */
	for(i=0;i<db->chip.n_wires;i++) {
		for(j=0;j<db->chip.wires[i].n_tile_wires;j++)
			free(db->chip.wires[i].tile_wires[j].pips);
		free(db->chip.wires[i].tile_wires);
	}
	free(db->chip.wires);
	
	/* tile types */
	for(i=0;i<db->n_tile_types;i++) {
		free(db->tile_types[i].name);
		free(db->tile_types[i].sites);
		for(j=0;j<db->tile_types[i].n_tile_wires;j++)
			free(db->tile_types[i].tile_wire_names[j]);
		free(db->tile_types[i].tile_wire_names);
	}
	free(db->tile_types);
	
	/* site types */
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

void db_alloc_tile(struct db *db, struct tile *tile)
{
	int i;
	struct tile_type *tt;
	struct site_type *st;
	
	tt = &db->tile_types[tile->type];
	tile->sites = alloc_size0(tt->n_sites*sizeof(struct site));
	for(i=0;i<tt->n_sites;i++) {
		st = &db->site_types[tt->sites[i]];
		tile->sites[i].input_wires = alloc_size0(st->n_inputs*sizeof(long int));
		tile->sites[i].output_wires = alloc_size0(st->n_outputs*sizeof(long int));
	}
}

int db_resolve_site(struct db *db, const char *name)
{
	int i;
	
	for(i=0;i<db->n_site_types;i++)
		if(strcmp(db->site_types[i].name, name) == 0)
			return i;
	return -1;
}

int db_resolve_tile(struct db *db, const char *name)
{
	int i;
	
	for(i=0;i<db->n_tile_types;i++)
		if(strcmp(db->tile_types[i].name, name) == 0)
			return i;
	return -1;
}

int db_resolve_input_pin(struct site_type *st, const char *name)
{
	int i;
	
	for(i=0;i<st->n_inputs;i++)
		if(strcmp(st->input_pin_names[i], name) == 0)
			return i;
	return -1;
}

int db_resolve_output_pin(struct site_type *st, const char *name)
{
	int i;
	
	for(i=0;i<st->n_outputs;i++)
		if(strcmp(st->output_pin_names[i], name) == 0)
			return i;
	return -1;
}

struct tile *db_lookup_tile(struct db *db, int type, int x, int y)
{
	int i;
	
	for(i=0;i<db->chip.w*db->chip.h;i++)
		if((db->chip.tiles[i].type == type) && (db->chip.tiles[i].x == x) && (db->chip.tiles[i].y == y))
			return &db->chip.tiles[i];
	return NULL;
}

int db_get_unused_site_in_tile(struct db *db, struct tile *tile, int st)
{
	int i;
	
	#ifdef DEBUG
	printf("Attempting to find a %s site in tile %s_X%dY%d (tile has %d sites)\n", db->site_types[st].name, db->tile_types[tile->type].name, tile->x, tile->y, db->tile_types[tile->type].n_sites);
	#endif
	for(i=0;i<db->tile_types[tile->type].n_sites;i++)
		if((db->tile_types[tile->type].sites[i] == st) && (tile->sites[i].name == NULL))
			return i;
	return -1;
}
