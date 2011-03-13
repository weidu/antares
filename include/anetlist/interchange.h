#ifndef __ANETLIST_INTERCHANGE_H
#define __ANETLIST_INTERCHANGE_H

#include <stdio.h>
#include <anetlist/net.h>

typedef struct anetlist_entity * (*anetlist_find_entity_c)(const char *);

struct anetlist *anetlist_parse_fd(FILE *fd, anetlist_find_entity_c find_entity_c);
void anetlist_write_fd(struct anetlist *a, FILE *fd);

struct anetlist *anetlist_parse_file(const char *filename, anetlist_find_entity_c find_entity_c);
void anetlist_write_file(struct anetlist *a, const char *filename);

#endif /* __ANETLIST_INTERCHANGE_H */
