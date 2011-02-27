#include <stdio.h>
#include <stdlib.h>

#include <chip/db.h>
#include <banner/banner.h>

#include "xdlrc.h"

int main(int argc, char *argv[])
{
	struct db *db;
	struct xdlrc_tokenizer *t;
	char *s;
	
	if(argc != 2) {
		fprintf(stderr, "Usage: antares-mkdb <file.xdlrc>\n");
		exit(EXIT_FAILURE);
	}
	
	t = xdlrc_create_tokenizer(argv[1]);
	while(1) {
		s = xdlrc_get_token(t);
		if(s == NULL)
			break;
		printf("token: '%s'\n", s);
		free(s);
	}
	xdlrc_free_tokenizer(t);
	
	db = db_create(20, 14, 7, 76);
	db_free(db);
	
	return 0;
}
