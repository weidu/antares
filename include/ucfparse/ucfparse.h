#ifndef __UCFPARSE_UCFPARSE_H
#define __UCFPARSE_UCFPARSE_H

#include <stdio.h>

enum {
	UCF_ATTR_LOC,
	UCF_ATTR_IOSTANDARD,
	UCF_ATTR_SLEW,
	UCF_ATTR_DRIVE
};

struct ucfparse_attr {
	int attr;
	char *value;
	struct ucfparse_attr *next;
};

struct ucfparse_net {
	char *name;
	struct ucfparse_attr *attrs;
	struct ucfparse_net *next;
};

struct ucfparse {
	struct ucfparse_net *nets;
};

struct ucfparse *ucfparse_fd(FILE *fd);
void ucfparse_free(struct ucfparse *u);
struct ucfparse *ucfparse_file(const char *filename);

#endif /* __UCFPARSE_UCFPARSE_H */
