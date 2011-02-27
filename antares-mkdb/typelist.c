#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <chip/db.h>

#include "xdlrc.h"

static struct db *create_db_tiles(struct xdlrc_tokenizer *t)
{
	xdlrc_close_parenthese(t);
	return db_create(20, 14, 7, 76);
}

struct db *typelist_create_db(const char *filename)
{
	struct db *db;
	struct xdlrc_tokenizer *t;
	char *s;
	char *chip;
	
	db = NULL;
	
	t = xdlrc_create_tokenizer(filename);

	xdlrc_get_token_par(t, 1);
	s = xdlrc_get_token_noeof(t);
	if(strcmp(s, "xdl_resource_report") != 0) {
		fprintf(stderr, "Expected xdl_resource_report, got '%s'\n", s);
		exit(EXIT_FAILURE);
	}
	free(s);

	s = xdlrc_get_token_noeof(t);
	printf("XDLRC version: %s\n", s);
	free(s);
	chip = xdlrc_get_token_noeof(t);
	printf("Chip: %s\n", chip);
	s = xdlrc_get_token_noeof(t);
	printf("Family: %s\n", s);
	free(s);

	while(1) {
		s = xdlrc_get_token_noeof(t);
		if(strcmp(s, "(") == 0) {
			free(s);
			s = xdlrc_get_token(t);
			if(strcmp(s, "tiles") == 0)
				db = create_db_tiles(t);
			else
				xdlrc_close_parenthese(t);
			free(s);
		} else if(strcmp(s, ")") == 0) {
			free(s);
			break;
		} else {
			fprintf(stderr, "Expected (, got %s\n", s);
			exit(EXIT_FAILURE);
		}
	}

	xdlrc_free_tokenizer(t);
	
	if(db == NULL) {
		fprintf(stderr, "Empty file!\n");
		exit(EXIT_FAILURE);
	}
	db->chip_ref = chip;
	
	return db;
}
