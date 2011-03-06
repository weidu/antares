#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chip/db.h>
#include <chip/load.h>

#include "route.h"

static void resolve_site_pin(struct db *db, const char *site_name, const char *pin_name, struct wire **w)
{
	int r;
	struct tile *tile;
	int site_index;
	struct site_type *st;
	int output;
	int pin_index;
	
	r = db_lookup_site(db, site_name, &tile, &site_index);
	if(!r) {
		fprintf(stderr, "Site %s not found\n", site_name);
		exit(EXIT_FAILURE);
	}
	st = &db->site_types[db->tile_types[tile->type].sites[site_index]];
	r = db_lookup_pin(st, pin_name, &output, &pin_index);
	if(!r) {
		fprintf(stderr, "Pin %s not found\n", pin_name);
		exit(EXIT_FAILURE);
	}
	printf("  Tile: %s_X%dY%d\n", db->tile_types[tile->type].name, tile->x, tile->y);
	printf("  Site type: %s\n", st->name);
	printf("  Pin is an %s connected to", output ? "output" : "input");
	if(output)
		*w = &db->chip.wires[tile->sites[site_index].output_wires[pin_index]];
	else
		*w = &db->chip.wires[tile->sites[site_index].input_wires[pin_index]];
	print_wire(db, *w, NULL);
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct db *db;
	char *dbfile, *start_site, *start_pin, *end_site, *end_pin;
	struct wire *start_wire, *end_wire;

	if(argc != 6) {
		fprintf(stderr, "Usage: test-router <db.acg> <start_site> <start_pin> <end_site> <end_pin>\n");
		exit(EXIT_FAILURE);
	}
	
	dbfile = argv[1];
	start_site = argv[2];
	start_pin = argv[3];
	end_site = argv[4];
	end_pin = argv[5];

	printf("Reading chip database...\n");
	db = db_load_file(dbfile);
	printf("...done.\n\n");
	
	printf("Chip: %s\n", db->chip_ref);
	printf("Grid: %dx%d\n", db->chip.w, db->chip.h);
	printf("Wires: %d\n", db->chip.n_wires);
	printf("Tile types: %d\n", db->n_tile_types);
	printf("Site types: %d\n\n", db->n_site_types);
	
	printf("Start:\n");
	resolve_site_pin(db, start_site, start_pin, &start_wire);
	printf("End:\n");
	resolve_site_pin(db, end_site, end_pin, &end_wire);
	
	printf("\n");
	route(db, start_wire, end_wire);
	
	db_free(db);
	
	return 0;
}
