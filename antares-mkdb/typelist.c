#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include <chip/db.h>

#include "xdlrc.h"

struct e_name {
	char *name;
	struct e_name *next;
};

struct e_site_type;

struct e_tile_site {
	struct e_site_type *type;
	struct e_tile_site *next;
};

struct e_tile_type {
	char *name;
	int n_tile_sites;
	struct e_tile_site *shead;
	int n_tile_wires;
	struct e_name *whead;
	struct e_tile_type *next;
};

struct e_site_type {
	char *name;
	int n_inputs;
	struct e_name *ihead;
	int n_outputs;
	struct e_name *ohead;
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

static void free_e_name(struct e_name *head)
{
	struct e_name *n1, *n2;
	
	n1 = head;
	while(n1 != NULL) {
		n2 = n1->next;
		free(n1);
		n1 = n2;
	}
}

static void free_site_type(struct e_site_type *st)
{
	free_e_name(st->ihead);
	free_e_name(st->ohead);
	free(st->name);
	free(st);
}

static void free_tile_type(struct e_tile_type *tt)
{
	struct e_tile_site *ts1, *ts2;
	
	ts1 = tt->shead;
	while(ts1 != NULL) {
		ts2 = ts1->next;
		free(ts1);
		ts1 = ts2;
	}
	free_e_name(tt->whead);
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
	
	#ifdef DEBUG
	printf("Tile type: %s\n", t);
	#endif
	
	tt = alloc_type(struct e_tile_type);
	tt->name = stralloc(t);
	tt->n_tile_sites = 0;
	tt->shead = NULL;
	tt->n_tile_wires = 0;
	tt->whead = NULL;
	tt->next = reg->thead;
	reg->thead = tt;
	reg->n_tile_types++;
	
	return tt;
}

static struct e_name *register_wire(struct e_tile_type *tt, char *t)
{
	struct e_name *wn;
	
	wn = tt->whead;
	while(wn != NULL) {
		if(strcmp(wn->name, t) == 0)
			return wn;
		wn = wn->next;
	}
	
	#ifdef DEBUG
	printf("Tile: %s Wire: %s\n", tt->name, t);
	#endif
	
	wn = alloc_type(struct e_name);
	wn->name = stralloc(t);
	wn->next = tt->whead;
	tt->whead = wn;
	tt->n_tile_wires++;
	
	return wn;
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
	
	#ifdef DEBUG
	printf("Site type: %s\n", t);
	#endif
	
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

static struct e_tile_site *register_tile_site(struct e_tile_type *tt, struct e_site_type *st)
{
	struct e_tile_site *ts;
	
	ts = tt->shead;
	while(ts != NULL) {
		if(ts->type == st)
			return ts;
		ts = ts->next;
	}
	
	#ifdef DEBUG
	printf("Tile: %s Site: %s\n", tt->name, st->name);
	#endif
	
	ts = alloc_type(struct e_tile_site);
	ts->type = st;
	ts->next = tt->shead;
	tt->shead = ts;
	tt->n_tile_sites++;
	
	return ts;
}

static struct e_name *register_site_output(struct e_site_type *st, char *t)
{
	struct e_name *pn;
	
	pn = st->ohead;
	while(pn != NULL) {
		if(strcmp(pn->name, t) == 0)
			return pn;
		pn = pn->next;
	}
	
	#ifdef DEBUG
	printf("Site: %s Output: %s\n", st->name, t);
	#endif
	
	pn = alloc_type(struct e_name);
	pn->name = stralloc(t);
	pn->next = st->ohead;
	st->ohead = pn;
	st->n_outputs++;
	
	return pn;
}

static struct e_name *register_site_input(struct e_site_type *st, char *t)
{
	struct e_name *pn;
	
	pn = st->ihead;
	while(pn != NULL) {
		if(strcmp(pn->name, t) == 0)
			return pn;
		pn = pn->next;
	}
	
	#ifdef DEBUG
	printf("Site: %s Input: %s\n", st->name, t);
	#endif
	
	pn = alloc_type(struct e_name);
	pn->name = stralloc(t);
	pn->next = st->ihead;
	st->ihead = pn;
	st->n_inputs++;
	
	return pn;
}

static void handle_primitive_site(struct registry *reg, struct e_tile_type *tt, struct xdlrc_tokenizer *t)
{
	char *s;
	struct e_site_type *st;
	
	free(xdlrc_get_token_noeof(t)); /* Name, e.g. "BUFIO2_X4Y26" */
	s = xdlrc_get_token_noeof(t); /* Type */
	st = register_site_type(reg, s);
	register_tile_site(tt, st);
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
				free(xdlrc_get_token_noeof(t)); /* Pinwire name. Registered later as regular tile wire. */
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

static void handle_wire(struct registry *reg, struct e_tile_type *tt, struct xdlrc_tokenizer *t)
{
	char *s;
	
	s = xdlrc_get_token_noeof(t);
	register_wire(tt, s);
	free(s);
	xdlrc_close_parenthese(t);
}

static void transfer_to_db(struct db *db, struct registry *reg)
{
	int i, j;
	struct e_site_type *st;
	struct e_tile_type *tt;
	struct e_tile_site *ts;
	struct e_name *w;
	
	/* 1. Transfer site types */
	st = reg->shead;
	for(i=0;i<reg->n_site_types;i++) {
		db->site_types[i].name = stralloc(st->name);
		db->site_types[i].n_inputs = st->n_inputs;
		db->site_types[i].input_pin_names = alloc_size(st->n_inputs*sizeof(char *));
		w = st->ihead;
		for(j=0;j<st->n_inputs;j++) {
			db->site_types[i].input_pin_names[j] = stralloc(w->name);
			w = w->next;
		}
		db->site_types[i].n_outputs = st->n_outputs;
		db->site_types[i].output_pin_names = alloc_size(st->n_outputs*sizeof(char *));
		w = st->ohead;
		for(j=0;j<st->n_outputs;j++) {
			db->site_types[i].output_pin_names[j] = stralloc(w->name);
			w = w->next;
		}
		st = st->next;
	}
	
	/* 2. Transfer tile types (site type indexes in DB are needed for this) */
	tt = reg->thead;
	for(i=0;i<reg->n_tile_types;i++) {
		db->tile_types[i].name = stralloc(tt->name);
		db->tile_types[i].n_sites = tt->n_tile_sites;
		db->tile_types[i].sites = alloc_size(tt->n_tile_sites*sizeof(int));
		ts = tt->shead;
		for(j=0;j<tt->n_tile_sites;j++) {
			db->tile_types[i].sites[j] = db_resolve_site(db, ts->type->name);
			assert(db->tile_types[i].sites[j] != -1);
			ts = ts->next;
		}
		db->tile_types[i].n_tile_wires = tt->n_tile_wires;
		db->tile_types[i].tile_wire_names = alloc_size(tt->n_tile_wires*sizeof(char *));
		w = tt->whead;
		for(j=0;j<tt->n_tile_wires;j++) {
			db->tile_types[i].tile_wire_names[j] = stralloc(w->name);
			w = w->next;
		}
		tt = tt->next;
	}
}

static struct db *create_db_tiles(struct xdlrc_tokenizer *t)
{
	struct registry *reg;
	int w, h;
	int i;
	char *s;
	struct e_tile_type *tt;
	struct db *db;
	
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
		tt = register_tile_type(reg, s);
		free(s);
		xdlrc_get_token_int(t); /* number of sites */
		while(1) {
			s = xdlrc_get_token_noeof(t);
			if(strcmp(s, "(") == 0) {
				free(s);
				s = xdlrc_get_token_noeof(t);
				if(strcmp(s, "primitive_site") == 0)
					handle_primitive_site(reg, tt, t);
				else if(strcmp(s, "wire") == 0)
					handle_wire(reg, tt, t);
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
	
	#ifdef DEBUG
	printf("n_tile_types=%d n_site_types=%d w=%d h=%d\n", reg->n_tile_types, reg->n_site_types, w, h);
	#endif
	db = db_create(reg->n_tile_types, reg->n_site_types, w, h);
	
	transfer_to_db(db, reg);
	
	free_registry(reg);
	
	return db;
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
	#ifdef DEBUG
	printf("XDLRC version: %s\n", s);
	#endif
	free(s);
	chip = xdlrc_get_token_noeof(t);
	#ifdef DEBUG
	printf("Chip: %s\n", chip);
	#endif
	s = xdlrc_get_token_noeof(t);
	#ifdef DEBUG
	printf("Family: %s\n", s);
	#endif
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
