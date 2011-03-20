#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <util.h>

#include <banner/banner.h>

#include <anetlist/net.h>
#include <anetlist/interchange.h>
#include <anetlist/entities.h>

#include <chip/db.h>
#include <chip/load.h>

#include <ucfparse/ucfparse.h>

#include <config.h>

#include "resmgr.h"
#include "constraints.h"
#include "ucf.h"
#include "initial.h"

static void help()
{
	banner("Placer");
	printf("Usage: antares-place [parameters] <input.anl>\n");
	printf("The packed input design in Antares netlist format (input.anl) is mandatory.\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -o <output.anl> Set the name of the output file.\n");
	printf("  -u <constraints.ucf> Use the specified constraints file.\n");
	printf("  -d <db.anl.gz> Override the default chip database file.\n");
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
	r = asprintf(&out, "%s-placed.anl", inname);
	if(r == -1) abort();
	free(inname);
	return out;
}

static char *mk_dbname(char *chipname)
{
	char *c;
	int r;
	char *out;
	
	chipname = stralloc(chipname);
	c = strrchr(chipname, '-');
	if(c != NULL)
		*c = 0;
	r = asprintf(&out, ANTARES_INSTALL_PREFIX"/share/antares/%s.acg.gz", chipname);
	if(r == -1) abort();
	free(chipname);
	return out;
}

int main(int argc, char *argv[])
{
	int opt;
	char *inname;
	char *outname;
	char *ucfname;
	char *dbname;
	struct anetlist *a;
	struct db *db;
	struct resmgr *r;
	
	outname = NULL;
	ucfname = NULL;
	dbname = NULL;
	while((opt = getopt(argc, argv, "ho:u:d:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 'o':
				free(outname);
				outname = stralloc(optarg);
				break;
			case 'u':
				free(ucfname);
				ucfname = stralloc(optarg);
				break;
			case 'd':
				free(dbname);
				dbname = stralloc(optarg);
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	
	if((argc - optind) != 1) {
		fprintf(stderr, "antares-place: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(outname == NULL)
		outname = mk_outname(inname);
	
	a = anetlist_parse_file(inname, entity_find_bel);

	if(dbname == NULL)
		dbname = mk_dbname(a->part_name);
	printf("Reading chip database %s...\n", dbname);
	db = db_load_file(dbname);
	printf("...done\n");
	
	r = resmgr_new(a, db);
	constraints_init(a);
	if(ucfname != NULL) {
		struct ucfparse *u;
		u = ucfparse_file(ucfname);
		ucf_apply(r, u);
		ucfparse_free(u);
	}
	constraints_infer_rel(a);
	initial_placement(r);
	
	constraints_free(a);
	resmgr_free(r);
	db_free(db);
	anetlist_free(a);
	
	free(outname);
	free(ucfname);
	free(dbname);

	return 0;
}
