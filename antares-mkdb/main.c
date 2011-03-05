#include <stdio.h>
#include <stdlib.h>

#include <chip/db.h>
#include <chip/store.h>
#include <banner/banner.h>

#include "typelist.h"
#include "chip.h"

int main(int argc, char *argv[])
{
	struct db *db;
	
	if(argc != 3) {
		fprintf(stderr, "Usage: antares-mkdb <input.xdlrc> <output.acg>\n");
		exit(EXIT_FAILURE);
	}
	
	printf("Listing types...\n");
	db = typelist_create_db(argv[1]);
	printf("Listing chip components...\n");
	chip_update_db(db, argv[1]);
	printf("Writing output file...\n");
	db_write_file(db, argv[2]);
	db_free(db);
	
	return 0;
}
