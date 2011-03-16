#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <util.h>

#include <chip/db.h>
#include <chip/load.h>

static unsigned char decode_char(gzFile fd)
{
	unsigned char x;
	gzread(fd, &x, 1);
	return x;
}

static unsigned short decode_short(gzFile fd)
{
	unsigned short x;
	gzread(fd, &x, 2);
	return x;
}

static int decode_int(gzFile fd)
{
	int x;
	gzread(fd, &x, 4);
	return x;
}

static char *decode_string(gzFile fd)
{
	int len;
	char *r;
	
	len = decode_char(fd);
	r = alloc_size(len+1);
	gzread(fd, r, len);
	r[len] = 0;
	return r;
}

static void decode_site(gzFile fd, struct site *site, int n_inputs, int n_outputs)
{
	int i;
	
	site->name = decode_string(fd);
	site->input_wires = alloc_size(n_inputs*sizeof(long int));
	for(i=0;i<n_inputs;i++)
		site->input_wires[i] = decode_int(fd);
	site->output_wires = alloc_size(n_outputs*sizeof(long int));
	for(i=0;i<n_outputs;i++)
		site->output_wires[i] = decode_int(fd);
}

static void decode_tile(gzFile fd, struct db *db, struct tile *tile)
{
	int i;
	struct tile_type *tt;
	struct site_type *st;
	
	tile->type = decode_short(fd);
	tt = &db->tile_types[tile->type];
	tile->x = decode_short(fd);
	tile->y = decode_short(fd);
	tile->sites = alloc_size(tt->n_sites*sizeof(struct site));
	for(i=0;i<tt->n_sites;i++) {
		st = &db->site_types[tt->sites[i]];
		decode_site(fd, &tile->sites[i], st->n_inputs, st->n_outputs);
	}
}

static void decode_tile_wire(gzFile fd, struct tile_wire *tw)
{
	tw->tile = decode_short(fd);
	tw->name = decode_short(fd);
}

static void decode_pip(gzFile fd, struct pip *p)
{
	p->tile = decode_short(fd);
	p->endpoint = decode_int(fd);
	p->bidir = decode_char(fd);
}

static void decode_wire(gzFile fd, struct wire *wire)
{
	int i;
	
	wire->n_tile_wires = decode_short(fd);
	wire->tile_wires = alloc_size(wire->n_tile_wires*sizeof(struct tile_wire));
	for(i=0;i<wire->n_tile_wires;i++)
		decode_tile_wire(fd, &wire->tile_wires[i]);
	wire->n_pips = decode_short(fd);
	wire->pips = alloc_size(wire->n_pips*sizeof(struct pip));
	for(i=0;i<wire->n_pips;i++)
		decode_pip(fd, &wire->pips[i]);
}

static void decode_chip(gzFile fd, struct db *db, struct chip *chip)
{
	int i;
	
	for(i=0;i<chip->w*chip->h;i++)
		decode_tile(fd, db, &chip->tiles[i]);
	chip->n_wires = decode_int(fd);
	chip->wires = alloc_size(chip->n_wires*sizeof(struct wire));
	for(i=0;i<chip->n_wires;i++)
		decode_wire(fd, &chip->wires[i]);
}

static void decode_tile_type(gzFile fd, struct tile_type *tt)
{
	int i;
	
	tt->name = decode_string(fd);
	tt->n_sites = decode_short(fd);
	tt->sites = alloc_size(tt->n_sites*sizeof(int));
	for(i=0;i<tt->n_sites;i++)
		tt->sites[i] = decode_short(fd);
	tt->n_tile_wires = decode_short(fd);
	tt->tile_wire_names = alloc_size(tt->n_tile_wires*sizeof(char *));
	for(i=0;i<tt->n_tile_wires;i++)
		tt->tile_wire_names[i] = decode_string(fd);
}

static void decode_site_type(gzFile fd, struct site_type *st)
{
	int i;
	
	st->name = decode_string(fd);
	st->n_inputs = decode_short(fd);
	st->input_pin_names = alloc_size(st->n_inputs*sizeof(char *));
	for(i=0;i<st->n_inputs;i++)
		st->input_pin_names[i] = decode_string(fd);
	st->n_outputs = decode_short(fd);
	st->output_pin_names = alloc_size(st->n_outputs*sizeof(char *));
	for(i=0;i<st->n_outputs;i++)
		st->output_pin_names[i] = decode_string(fd);
}

struct db *db_load_fd(gzFile fd)
{
	struct db *db;
	int n_tile_types;
	int n_site_types;
	int w;
	int h;
	int i;
	
	n_tile_types = decode_short(fd);
	n_site_types = decode_short(fd);
	w = decode_short(fd);
	h = decode_short(fd);
	db = db_create(n_tile_types, n_site_types, w, h);
	
	for(i=0;i<db->n_tile_types;i++)
		decode_tile_type(fd, &db->tile_types[i]);
	for(i=0;i<db->n_site_types;i++)
		decode_site_type(fd, &db->site_types[i]);
	decode_chip(fd, db, &db->chip);
	
	return db;
}

struct db *db_load_file(const char *filename)
{
	gzFile fd;
	struct db *db;
	
	fd = gzopen(filename, "rb");
	if(fd == NULL) {
		perror("Unable to open chip database");
		exit(EXIT_FAILURE);
	}
	db = db_load_fd(fd);
	gzclose(fd);
	return db;
}
