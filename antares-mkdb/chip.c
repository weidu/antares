#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <chip/db.h>

#include "xdlrc.h"
#include "chip.h"

static void handle_primitive_site(struct db *db, struct tile *tile, struct xdlrc_tokenizer *t)
{
	xdlrc_close_parenthese(t);
}

static void handle_wire(struct db *db, struct tile *tile, struct xdlrc_tokenizer *t)
{
	xdlrc_close_parenthese(t);
}

static void handle_pip(struct db *db, struct tile *tile, struct xdlrc_tokenizer *t)
{
	xdlrc_close_parenthese(t);
}

static void parse_tile_name(struct db *db, char *tile_name, int *type, int *x, int *y)
{
	char *c;
	
	c = strrchr(tile_name, '_');
	if(c == NULL) {
		fprintf(stderr, "Unexpected tile name: '%s'\n", tile_name);
		exit(EXIT_FAILURE);
	}
	*c = 0;
	*type = db_resolve_tile(db, tile_name);
	if(*type == -1) {
		fprintf(stderr, "Unexpected tile type: '%s'\n'", tile_name);
		exit(EXIT_FAILURE);
	}
	c++;
	if(*c != 'X') {
		fprintf(stderr, "Unexpected tile position (X): '%s'\n'", c);
		exit(EXIT_FAILURE);
	}
	c++;
	*x = strtoul(c, &c, 10);
	if(*c != 'Y') {
		fprintf(stderr, "Unexpected tile position (Y): '%s'\n'", c);
		exit(EXIT_FAILURE);
	}
	c++;
	*y = strtoul(c, &c, 10);
	if(*c != 0) {
		fprintf(stderr, "Unexpected tile position (end): '%s'\n'", c);
		exit(EXIT_FAILURE);
	}
}

static void handle_tiles(struct db *db, struct xdlrc_tokenizer *t)
{
	int x, y;
	char *s;
	int offset;
	
	xdlrc_get_token_int(t); /* h */
	xdlrc_get_token_int(t); /* w */
	for(y=0;y<db->chip.h;y++)
		for(x=0;x<db->chip.w;x++) {
			offset = db->chip.w*y+x;
			xdlrc_get_token_par(t, 1);
			s = xdlrc_get_token(t);
			if(strcmp(s, "tile") != 0) {
				fprintf(stderr, "Expected tile, got '%s'\n", s);
				exit(EXIT_FAILURE);
			}
			free(s);
			xdlrc_get_token_int(t); /* tile X */
			xdlrc_get_token_int(t); /* tile Y */
			s = xdlrc_get_token(t); /* tile name */
			parse_tile_name(db, s, &db->chip.tiles[offset].type, &db->chip.tiles[offset].x, &db->chip.tiles[offset].y);
			free(s);
			free(xdlrc_get_token(t)); /* tile type */
			xdlrc_get_token_int(t); /* number of sites */
			db->chip.tiles[offset].sites = alloc_size0(db->tile_types[db->chip.tiles[offset].type].n_sites);
			while(1) {
				s = xdlrc_get_token_noeof(t);
				if(strcmp(s, "(") == 0) {
					free(s);
					s = xdlrc_get_token_noeof(t);
					if(strcmp(s, "primitive_site") == 0)
						handle_primitive_site(db, &db->chip.tiles[offset], t);
					else if(strcmp(s, "wire") == 0)
						handle_wire(db, &db->chip.tiles[offset], t);
					else if(strcmp(s, "pip") == 0)
						handle_pip(db, &db->chip.tiles[offset], t);
					else
						xdlrc_close_parenthese(t);
					free(s);
				} else if(strcmp(s, ")") == 0) {
					free(s);
					break;
				} else {
					fprintf(stderr, "Expected parenthese, got '%s'\n", s);
					exit(EXIT_FAILURE);
				}
			}
		}
	xdlrc_close_parenthese(t);
}

void chip_update_db(struct db *db, const char *filename)
{
	struct xdlrc_tokenizer *t;
	char *s;
	
	t = xdlrc_create_tokenizer(filename);
	
	xdlrc_get_token_par(t, 1);
	s = xdlrc_get_token_noeof(t);
	if(strcmp(s, "xdl_resource_report") != 0) {
		fprintf(stderr, "Expected xdl_resource_report, got '%s'\n", s);
		exit(EXIT_FAILURE);
	}
	free(s);
	
	free(xdlrc_get_token_noeof(t)); /* Version */
	free(xdlrc_get_token_noeof(t)); /* Chip */
	free(xdlrc_get_token_noeof(t)); /* Family */

	while(1) {
		s = xdlrc_get_token_noeof(t);
		if(strcmp(s, "(") == 0) {
			free(s);
			s = xdlrc_get_token(t);
			if(strcmp(s, "tiles") == 0)
				handle_tiles(db, t);
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
}
