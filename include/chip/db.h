#ifndef __CHIP_DB_H
#define __CHIP_DB_H

struct wire;

struct site {
	struct wire *input_wires;	/* < array of wires the inputs are connected to */
	struct wire *output_wires;	/* < array of wires the outputs are connected to */
};

struct tile {
	int type;			/* < type of the tile (NULL, INT, CLEXM, ...) */
	int x;				/* < X coordinate of the tile */
	int y;				/* < Y coordinate of the tile. The XDL tile name is type_XxYy. */
	struct site *sites;		/* < array of sites */
};

struct pip {
	struct wire *endpoint;		/* < destination wire of this pip */
	int bidir;			/* < is this pip bidirectional? (nb. two entries are created) */
	struct pip *next;		/* < next pip in this tile with this wire as start point */
};

struct tile_wire {
	struct tile *tile;		/* < current tile */
	int name;			/* < how this wire is named in the current tile */
	struct pip *phead;		/* < head of the list of pips in this tile with this wire as start point */
	struct tile_wire *next;		/* < next tile wire for this wire */
};

struct wire {
	struct tile_wire *whead;	/* < first tile wire for this wire */
	struct wire *next;		/* < next wire in the chip */
};

struct chip {
	int w;				/* < number of tiles in the X direction */
	int h;				/* < number of tiles in the Y direction */
	struct tile *tiles;		/* < array of tiles */
	struct wire *whead;		/* < linked list of wires */
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

#endif
