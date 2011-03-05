#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chip/db.h>
#include <chip/load.h>

int main(int argc, char *argv[])
{
	struct db *db;
	char *dbfile, *start_site, *start_pin, *end_site, *end_pin;
	
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
	
	db_free(db);
	
	return 0;
}
