#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chip/db.h>
#include <chip/load.h>

static void list_sites(struct db *db)
{
	int clexm, clexl;
	int i;
	struct tile *tile;
	
	clexm = db_resolve_tile(db, "CLEXM");
	clexl = db_resolve_tile(db, "CLEXL");
	for(i=0;i<db->chip.w*db->chip.h;i++) {
		tile = &db->chip.tiles[i];
		if((tile->type == clexm) || (tile->type == clexl))
			printf("%s_X%dY%d %s\n", db->tile_types[tile->type].name, tile->x, tile->y,
				tile->sites[0].name);
	}
}

int main(int argc, char *argv[])
{
	struct db *db;
	char *dbfile;

	if(argc != 2) {
		fprintf(stderr, "Usage: test-sitelist <db.acg>\n");
		exit(EXIT_FAILURE);
	}
	
	dbfile = argv[1];

	printf("Reading chip database...\n");
	db = db_load_file(dbfile);
	printf("...done.\n\n");
	
	list_sites(db);
	
	db_free(db);
	
	return 0;
}
