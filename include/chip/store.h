#ifndef __CHIP_STORE_H
#define __CHIP_STORE_H

#include <zlib.h>
#include <chip/db.h>

void db_write_fd(struct db *db, gzFile fd);
void db_write_file(struct db *db, const char *filename);

#endif /* __CHIP_STORE_H */
