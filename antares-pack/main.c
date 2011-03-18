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

#include "transform.h"

static void help()
{
	banner("BEL packer");
	printf("Usage: antares-pack [parameters] <input.anl>\n");
	printf("The input design in Antares netlist format (input.anl) is mandatory.\n");
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
	r = asprintf(&out, "%s-packed.anl", inname);
	if(r == -1) abort();
	free(inname);
	return out;
}

int main(int argc, char *argv[])
{
	int opt;
	char *inname;
	char *outname;
	struct anetlist *src, *dst;

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
		fprintf(stderr, "antares-pack: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(outname == NULL)
		outname = mk_outname(inname);
	
	src = anetlist_parse_file(inname, entity_find_primitive);
	dst = anetlist_new();
	transform(src, dst);
	anetlist_write_file(dst, outname);
	anetlist_free(dst);
	anetlist_free(src);
	
	free(outname);

	return 0;
}
