#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <chip/db.h>

#include "xdlrc.h"

struct e_tile_type {
	char *name;
	struct e_tile_type *next;
};

struct e_pin_name {
	char *name;
	struct e_pin_name *next;
};

struct e_site_type {
	char *name;
	int n_inputs;
	struct e_pin_name *ihead;
	int n_outputs;
	struct e_pin_name *ohead;
	struct e_site_type *next;
};

struct registry {
	int n_tile_types;
	struct e_tile_type *thead;
	int n_site_types;
	struct e_site_type *shead;
};

static struct registry *create_registry()
{
	struct registry *reg;
	
	reg = alloc_type(struct registry);
	reg->n_tile_types = 0;
	reg->thead = NULL;
	reg->n_site_types = 0;
	reg->shead = NULL;
	
	return reg;
}

static void free_site_type(struct e_site_type *st)
{
	free(st->name);
	free(st);
}

static void free_tile_type(struct e_tile_type *tt)
{
	free(tt->name);
	free(tt);
}

static void free_registry(struct registry *reg)
{
	struct e_tile_type *tt1, *tt2;
	struct e_site_type *st1, *st2;
	
	tt1 = reg->thead;
	while(tt1 != NULL) {
		tt2 = tt1->next;
		free_tile_type(tt1);
		tt1 = tt2;
	}
	st1 = reg->shead;
	while(st1 != NULL) {
		st2 = st1->next;
		free_site_type(st1);
		st1 = st2;
	}
	free(reg);
}

static struct e_tile_type *register_tile_type(struct registry *reg, char *t)
{
	struct e_tile_type *tt;
	
	tt = reg->thead;
	while(tt != NULL) {
		if(strcmp(tt->name, t) == 0)
			return tt;
		tt = tt->next;
	}
	
	printf("Tile type: %s\n", t);
	
	tt = alloc_type(struct e_tile_type);
	tt->name = stralloc(t);
	tt->next = reg->thead;
	reg->thead = tt;
	reg->n_tile_types++;
	
	return tt;
}

static struct e_site_type *register_site_type(struct registry *reg, char *t)
{
	struct e_site_type *st;
	
	st = reg->shead;
	while(st != NULL) {
		if(strcmp(st->name, t) == 0)
			return st;
		st = st->next;
	}
	
	printf("Site type: %s\n", t);
	
	st = alloc_type(struct e_site_type);
	st->name = stralloc(t);
	st->n_inputs = 0;
	st->ihead = NULL;
	st->n_outputs = 0;
	st->ohead = NULL;
	st->next = reg->shead;
	reg->shead = st;
	reg->n_site_types++;
	
	return st;
}

static struct e_pin_name *register_site_output(struct e_site_type *st, char *t)
{
	struct e_pin_name *pn;
	
	pn = st->ohead;
	while(pn != NULL) {
		if(strcmp(pn->name, t) == 0)
			return pn;
		pn = pn->next;
	}
	
	printf("Site: %s Output: %s\n", st->name, t);
	
	pn = alloc_type(struct e_pin_name);
	pn->name = stralloc(t);
	pn->next = st->ohead;
	st->ohead = pn;
	st->n_outputs++;
	
	return pn;
}

static struct e_pin_name *register_site_input(struct e_site_type *st, char *t)
{
	struct e_pin_name *pn;
	
	pn = st->ihead;
	while(pn != NULL) {
		if(strcmp(pn->name, t) == 0)
			return pn;
		pn = pn->next;
	}
	
	printf("Site: %s Input: %s\n", st->name, t);
	
	pn = alloc_type(struct e_pin_name);
	pn->name = stralloc(t);
	pn->next = st->ihead;
	st->ihead = pn;
	st->n_inputs++;
	
	return pn;
}

static void handle_primitive_site(struct registry *reg, struct xdlrc_tokenizer *t)
{
	char *s;
	struct e_site_type *st;
	
	free(xdlrc_get_token_noeof(t)); /* Name, e.g. "BUFIO2_X4Y26" */
	s = xdlrc_get_token_noeof(t); /* Type */
	st = register_site_type(reg, s);
	free(s);
	free(xdlrc_get_token_noeof(t)); /* bonded/internal */
	xdlrc_get_token_int(t); /* Number of elements */
	while(1) {
		s = xdlrc_get_token_noeof(t);
		if(strcmp(s, "(") == 0) {
			free(s);
			s = xdlrc_get_token_noeof(t);
			if(strcmp(s, "pinwire") == 0) {
				char *pin_name;
				char *pin_type;
				
				pin_name = xdlrc_get_token_noeof(t);
				pin_type = xdlrc_get_token_noeof(t);
				free(xdlrc_get_token_noeof(t)); /* pinwire name */
				if(strcmp(pin_type, "input") == 0)
					register_site_input(st, pin_name);
				else if(strcmp(pin_type, "output") == 0)
					register_site_output(st, pin_name);
				else {
					fprintf(stderr, "Unknown site pin type: '%s'\n", pin_type);
					exit(EXIT_FAILURE);
				}
				free(pin_name);
				free(pin_type);
				xdlrc_get_token_par(t, 0);
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

static void handle_wire(struct registry *reg, struct e_tile_type *tile_type, struct xdlrc_tokenizer *t)
{
	xdlrc_close_parenthese(t);
}

static struct db *create_db_tiles(struct xdlrc_tokenizer *t)
{
	struct registry *reg;
	int w, h;
	int i;
	char *s;
	struct e_tile_type *tile_type;
	
	reg = create_registry();
	h = xdlrc_get_token_int(t);
	w = xdlrc_get_token_int(t);
	for(i=0;i<w*h;i++) {
		xdlrc_get_token_par(t, 1);
		s = xdlrc_get_token(t);
		if(strcmp(s, "tile") != 0) {
			fprintf(stderr, "Expected tile, got '%s'\n", s);
			exit(EXIT_FAILURE);
		}
		free(s);
		xdlrc_get_token_int(t); /* tile X */
		xdlrc_get_token_int(t); /* tile Y */
		free(xdlrc_get_token(t)); /* tile name */
		s = xdlrc_get_token(t); /* tile type */
		tile_type = register_tile_type(reg, s);
		free(s);
		xdlrc_get_token_int(t); /* number of sites */
		while(1) {
			s = xdlrc_get_token_noeof(t);
			if(strcmp(s, "(") == 0) {
				free(s);
				s = xdlrc_get_token_noeof(t);
				if(strcmp(s, "primitive_site") == 0)
					handle_primitive_site(reg, t);
				else if(strcmp(s, "wire") == 0)
					handle_wire(reg, tile_type, t);
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
	free_registry(reg);
	return db_create(20, 14, w, h);
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
			if(strcmp(s, "tiles") == 0) {
				if(db != NULL) {
					fprintf(stderr, "XDL contains several tiles declarations\n");
					exit(EXIT_FAILURE);
				}
				db = create_db_tiles(t);
			} else
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
