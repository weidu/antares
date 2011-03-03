#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <chip/db.h>

#include "xdlrc.h"
#include "connectivity.h"
#include "chip.h"

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

static void handle_primitive_site(struct db *db, struct conn *c, struct tile *tile, struct xdlrc_tokenizer *t)
{
	char *s;
	char *name;
	char *stype;
	int index;
	struct site *site;
	struct site_type *site_type;
	
	name = xdlrc_get_token_noeof(t);
	stype = xdlrc_get_token_noeof(t);
	index = db_get_unused_site_in_tile(db, tile, db_resolve_site(db, stype));
	if(index == -1) {
		fprintf(stderr, "Failed to find a free '%s' site in tile\n", stype);
		exit(EXIT_FAILURE);
	}
	free(stype);
	site = &tile->sites[index];
	site_type = &db->site_types[db->tile_types[tile->type].sites[index]];
	site->name = name;
	
	free(xdlrc_get_token_noeof(t)); /* bonded/unbonded/internal */
	xdlrc_get_token_int(t); /* number of pins */
	
	while(1) {
		s = xdlrc_get_token_noeof(t);
		if(strcmp(s, "(") == 0) {
			free(s);
			s = xdlrc_get_token_noeof(t);
			if(strcmp(s, "pinwire") == 0) {
				char *pin_name;
				char *pin_type;
				char *pinwire_name;
				
				pin_name = xdlrc_get_token_noeof(t);
				pin_type = xdlrc_get_token_noeof(t);
				pinwire_name = xdlrc_get_token_noeof(t);
				xdlrc_get_token_par(t, 0);
				if(strcmp(pin_type, "input") == 0) {
					index = db_resolve_input_pin(site_type, pin_name);
					assert(index != -1);
					site->input_wires[index] = (long int)conn_get_wire_tile(c, tile, pinwire_name);
				} else if(strcmp(pin_type, "output") == 0) {
					index = db_resolve_output_pin(site_type, pin_name);
					assert(index != -1);
					site->output_wires[index] = (long int)conn_get_wire_tile(c, tile, pinwire_name);
				} else {
					fprintf(stderr, "Unknown site pin type: '%s'\n", pin_type);
					exit(EXIT_FAILURE);
				}
				free(pin_name);
				free(pin_type);
				free(pinwire_name);
			} else
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

static void handle_wire(struct db *db, struct conn *c, struct tile *tile, struct xdlrc_tokenizer *t)
{
	char *name;
	struct c_wire *wire;
	char *s;
	struct c_wire *wire2;
	int type, x, y;
	struct tile *neighbor;
	
	name = xdlrc_get_token_noeof(t);
	wire = conn_get_wire_tile(c, tile, name);
	free(name);
	xdlrc_get_token_int(t);
	while(1) {
		s = xdlrc_get_token_noeof(t);
		if(strcmp(s, "(") == 0) {
			free(s);
			s = xdlrc_get_token_noeof(t);
			if(strcmp(s, "conn") == 0) {
				name = xdlrc_get_token_noeof(t);
				parse_tile_name(db, name, &type, &x, &y);
				free(name);
				neighbor = db_lookup_tile(db, type, x, y);
				if(neighbor == NULL) {
					fprintf(stderr, "Failed to find neighboring tile\n");
					exit(EXIT_FAILURE);
				}
				name = xdlrc_get_token_noeof(t);
				wire2 = conn_get_wire_tile(c, neighbor, name);
				free(name);
				xdlrc_get_token_par(t, 0);
				conn_join_wires(wire, wire2);
			} else
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

static void handle_pip(struct db *db, struct conn *c, struct tile *tile, struct xdlrc_tokenizer *t)
{
	char *source;
	char *dest;
	int dir;
	struct c_wire *w1, *w2;
	
	free(xdlrc_get_token_noeof(t)); /* tile name, repeated */
	source = xdlrc_get_token_noeof(t);
	dir = xdlrc_get_token_dir(t);
	dest = xdlrc_get_token_noeof(t);
	/* TODO: ignore route through "virtual PIPs" */
	xdlrc_close_parenthese(t);
	w1 = conn_get_wire_tile(c, tile, source);
	w2 = conn_get_wire_tile(c, tile, dest);
	free(source);
	free(dest);
	
	dir = dir != XDLRC_DIR_BUFFERED;
	conn_add_pip(tile, dir, w1, w2);
	if(dir)
		conn_add_pip(tile, dir, w2, w1);
}

static void handle_tiles(struct db *db, struct conn *c, struct xdlrc_tokenizer *t, int pass)
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
			db_alloc_tile(db, &db->chip.tiles[offset]);
			free(xdlrc_get_token(t)); /* tile type */
			xdlrc_get_token_int(t); /* number of sites */
			while(1) {
				s = xdlrc_get_token_noeof(t);
				if(strcmp(s, "(") == 0) {
					free(s);
					s = xdlrc_get_token_noeof(t);
					if(!pass && (strcmp(s, "primitive_site") == 0))
						handle_primitive_site(db, c, &db->chip.tiles[offset], t);
					else if(pass && (strcmp(s, "wire") == 0))
						handle_wire(db, c, &db->chip.tiles[offset], t);
					else if(!pass && (strcmp(s, "pip") == 0))
						handle_pip(db, c, &db->chip.tiles[offset], t);
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
	struct conn *c;
	int i;
	
	c = conn_create(db);;
	
	for(i=0;i<2;i++) {
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
					handle_tiles(db, c, t, i);
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
	
	/* TODO: transfer to DB */
	
	conn_free(c);
}
