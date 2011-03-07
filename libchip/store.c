#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <chip/db.h>
#include <chip/store.h>

static void encode_char(gzFile fd, unsigned char x)
{
	gzwrite(fd, &x, 1);
}

static void encode_short(gzFile fd, unsigned short x)
{
	gzwrite(fd, &x, 2);
}

static void encode_int(gzFile fd, int x)
{
	gzwrite(fd, &x, 4);
}

static void encode_string(gzFile fd, const char *str)
{
	int len;
	
	len = strlen(str);
	encode_char(fd, len);
	gzwrite(fd, str, len);
}

static void encode_site(gzFile fd, struct site *site, int n_inputs, int n_outputs)
{
	int i;
	
	encode_string(fd, site->name);
	for(i=0;i<n_inputs;i++)
		encode_int(fd, site->input_wires[i]);
	for(i=0;i<n_outputs;i++)
		encode_int(fd, site->output_wires[i]);
}

static void encode_tile(gzFile fd, struct db *db, struct tile *tile)
{
	int i;
	struct tile_type *tt;
	struct site_type *st;
	
	tt = &db->tile_types[tile->type];
	encode_short(fd, tile->type);
	encode_short(fd, tile->x);
	encode_short(fd, tile->y);
	/* number of sites is implied by type */
	for(i=0;i<tt->n_sites;i++) {
		st = &db->site_types[tt->sites[i]];
		encode_site(fd, &tile->sites[i], st->n_inputs, st->n_outputs);
	}
}

static void encode_tile_wire(gzFile fd, struct tile_wire *tw)
{
	encode_short(fd, tw->tile);
	encode_short(fd, tw->name);
}

static void encode_pip(gzFile fd, struct pip *p)
{
	encode_short(fd, p->tile);
	encode_int(fd, p->endpoint);
	encode_char(fd, p->bidir);
}

static void encode_wire(gzFile fd, struct wire *wire)
{
	int i;
	
	encode_short(fd, wire->n_tile_wires);
	for(i=0;i<wire->n_tile_wires;i++)
		encode_tile_wire(fd, &wire->tile_wires[i]);
	encode_short(fd, wire->n_pips);
	for(i=0;i<wire->n_pips;i++)
		encode_pip(fd, &wire->pips[i]);
}

static void encode_chip(gzFile fd, struct db *db, struct chip *chip)
{
	int i;
	
	for(i=0;i<chip->w*chip->h;i++)
		encode_tile(fd, db, &chip->tiles[i]);
	encode_int(fd, chip->n_wires);
	for(i=0;i<chip->n_wires;i++)
		encode_wire(fd, &chip->wires[i]);
}

static void encode_tile_type(gzFile fd, struct tile_type *tt)
{
	int i;
	
	encode_string(fd, tt->name);
	encode_short(fd, tt->n_sites);
	for(i=0;i<tt->n_sites;i++)
		encode_short(fd, tt->sites[i]);
	encode_short(fd, tt->n_tile_wires);
	for(i=0;i<tt->n_tile_wires;i++)
		encode_string(fd, tt->tile_wire_names[i]);
}

static void encode_site_type(gzFile fd, struct site_type *st)
{
	int i;
	
	encode_string(fd, st->name);
	encode_short(fd, st->n_inputs);
	for(i=0;i<st->n_inputs;i++)
		encode_string(fd, st->input_pin_names[i]);
	encode_short(fd, st->n_outputs);
	for(i=0;i<st->n_outputs;i++)
		encode_string(fd, st->output_pin_names[i]);
}

void db_write_fd(struct db *db, gzFile fd)
{
	int i;
	
	encode_short(fd, db->n_tile_types);
	encode_short(fd, db->n_site_types);
	encode_short(fd, db->chip.w);
	encode_short(fd, db->chip.h);
	encode_string(fd, db->chip_ref);
	for(i=0;i<db->n_tile_types;i++)
		encode_tile_type(fd, &db->tile_types[i]);
	for(i=0;i<db->n_site_types;i++)
		encode_site_type(fd, &db->site_types[i]);
	encode_chip(fd, db, &db->chip);
}

void db_write_file(struct db *db, const char *filename)
{
	gzFile fd;
	int r;

	fd = gzopen(filename, "wb");
	if(fd == NULL) {
		perror("db_write_file");
		exit(EXIT_FAILURE);
	}
	db_write_fd(db, fd);
	r = gzclose(fd);
	if(r != Z_OK) {
		perror("db_write_file");
		exit(EXIT_FAILURE);
	}
}
