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

static void help()
{
	banner("Router");
	printf("Usage: antares-route [parameters] <input.anl>\n");
	printf("The placed input design in Antares netlist format (input.anl) is mandatory.\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -o <output.anl> Set the name of the output file.\n");
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
	r = asprintf(&out, "%s-routed.anl", inname);
	if(r == -1) abort();
	free(inname);
	return out;
}

int main(int argc, char *argv[])
{
	int opt;
	char *inname;
	char *outname;
	struct anetlist *a;

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
		fprintf(stderr, "antares-route: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(outname == NULL)
		outname = mk_outname(inname);
	
	a = anetlist_parse_file(inname, entity_find_site);
	anetlist_free(a);
	
	free(outname);

	return 0;
}
