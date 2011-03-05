#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chip/db.h>
#include <chip/store.h>
#include <banner/banner.h>

#include "typelist.h"
#include "chip.h"

static char *mk_outname(char *inname)
{
	char *c;
	int r;
	char *out;
	
	c = strrchr(inname, '.');
	if(c != NULL)
		*c = 0;
	r = asprintf(&out, "%s.acg", inname);
	if(r == -1) abort();
	return out;
}

int main(int argc, char *argv[])
{
	struct db *db;
	char *inname;
	char *outname;
	
	if(argc != 2) {
		fprintf(stderr, "Usage: antares-mkdb <input.xdlrc>\n");
		exit(EXIT_FAILURE);
	}
	
	inname = argv[1];
	printf("Listing types...\n");
	db = typelist_create_db(inname);
	printf("Listing chip components...\n");
	chip_update_db(db, inname);
	printf("Writing output file...\n");
	outname = mk_outname(inname);
	db_write_file(db, outname);
	free(outname);
	db_free(db);
	
	return 0;
}
