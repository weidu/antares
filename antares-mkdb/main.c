#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <chip/db.h>
#include <chip/store.h>
#include <banner/banner.h>

#include "typelist.h"
#include "chip.h"

static void help()
{
	banner("Chip geometry compiler");
	printf("Usage: antares-mkdb [parameters] <input.xdlrc>\n\n");
	printf("The input database in XDLRC format (input.xdlrc) is mandatory.\n");
	printf("To create such a file with Xilinx ISE, use:\n");
	printf("  xdl -report -pips <chip>\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -o <output.acg.gz> Set the name of the output file.\n");
}

static char *mk_outname(char *inname)
{
	char *c;
	int r;
	char *out;
	
	inname = stralloc(inname);
	c = strrchr(inname, '.');
	if(c != NULL)
		*c = 0;
	r = asprintf(&out, "%s.acg.gz", inname);
	if(r == -1) abort();
	free(inname);
	return out;
}

int main(int argc, char *argv[])
{
	struct db *db;
	int opt;
	char *inname;
	char *outname;
	
	outname = NULL;
	
	while((opt = getopt(argc, argv, "ho:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 'o':
				free(outname);
				outname = stralloc(optarg);
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	
	if((argc - optind) != 1) {
		fprintf(stderr, "antares-mkdb: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(outname == NULL)
		outname = mk_outname(inname);
	
	db = typelist_create_db(inname);
	chip_update_db(db, inname);
	db_write_file(db, outname);
	db_free(db);
	
	free(outname);
	
	return 0;
}
