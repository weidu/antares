#ifndef __CHIP_LOAD_H
#define __CHIP_LOAD_H

#include <zlib.h>
#include <chip/db.h>

struct db *db_load_fd(gzFile fd);
struct db *db_load_file(const char *filename);

#endif /* __CHIP_LOAD_H */
