#ifndef __CHIP_DB_H
#define __CHIP_DB_H

struct wire;

/* HACK: we use long ints here so they can be casted to and from pointers even on 64-bit platforms */
struct site {
	char *name;			/* < "A3", "G5", "SLICE_X2Y125", ... */
	long int *input_wires;		/* < array of wires (offsets in the wire array) the inputs are connected to */
	long int *output_wires;		/* < array of wires (offsets in the wire array) the outputs are connected to */
};

struct tile {
	int type;			/* < type of the tile (NULL, INT, CLEXM, ...) */
	int x;				/* < X coordinate of the tile */
	int y;				/* < Y coordinate of the tile. The XDL tile name is type_XxYy. */
	struct site *sites;		/* < array of sites */
};

struct pip {
	int endpoint;			/* < destination wire of this pip (offset in the chip's wire array) */
	int bidir;			/* < is this pip bidirectional? (nb. two pip entries are created if yes) */
};

struct tile_wire {
	int tile;			/* < current tile (offset in the chip's tiles array) */
	int name;			/* < how this wire is named in the current tile */
	int n_pips;			/* < number of pips with this wire as start point */
	struct pip *pips;		/* < array of those pips */
};

struct wire {
	int n_tile_wires;		/* < number of tile wires this wire is made of */
	struct tile_wire *tile_wires;	/* < array of tile wires */
};

struct chip {
	int w;				/* < number of tiles in the X direction */
	int h;				/* < number of tiles in the Y direction */
	struct tile *tiles;		/* < array of tiles */
	int n_wires;			/* < number of wires */
	struct wire *wires;		/* < array of wires */
};

struct tile_type {
	char *name;			/* < "NULL", "INT", "CLEXM", etc. */
	int n_sites;			/* < number of primitive sites in each tile of this type */
	int *sites;			/* < type of each primitive site */
	int n_tile_wires;		/* < number of tile wires for this tile type */
	char **tile_wire_names;		/* < names of the tile wires */
};

struct site_type {
	char *name;			/* < "SLICEX", "SLICEM", etc. */
	int n_inputs;			/* < number of input pinwires */
	char **input_pin_names;		/* < names of the pins attached to input pinwires */
	int n_outputs;			/* < number of output pinwires */
	char **output_pin_names;	/* < names of the pins attached to output pinwires */
};

struct db {
	char *chip_ref;
	struct chip chip;
	int n_tile_types;
	struct tile_type *tile_types;
	int n_site_types;
	struct site_type *site_types;
};

struct db *db_create(int n_tile_types, int n_site_types, int w, int h);
void db_free(struct db *db);
void db_alloc_tile(struct db *db, struct tile *tile);

int db_resolve_site(struct db *db, const char *name);
int db_resolve_tile(struct db *db, const char *name);
int db_resolve_input_pin(struct site_type *st, const char *name);
int db_resolve_output_pin(struct site_type *st, const char *name);

struct tile *db_lookup_tile(struct db *db, int type, int x, int y);

int db_get_unused_site_in_tile(struct db *db, struct tile *tile, int st);

#endif
