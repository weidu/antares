#ifndef __XDLRC_H
#define __XDLRC_H

struct xdlrc_tokenizer {
	char *line;
	char *start;
	size_t n;
	FILE *fd;
};

enum {
	XDLRC_BIDIR_UNBUFFERED,
	XDLRC_BIDIR_BUFFERED1,
	XDLRC_BIDIR_BUFFERED2,
	XDLRC_DIR_BUFFERED
};

struct xdlrc_tokenizer *xdlrc_create_tokenizer(const char *filename);
void xdlrc_free_tokenizer(struct xdlrc_tokenizer *t);
char *xdlrc_get_token(struct xdlrc_tokenizer *t);
void xdlrc_get_token_par(struct xdlrc_tokenizer *t, int open);
int xdlrc_get_token_int(struct xdlrc_tokenizer *t);
int xdlrc_get_token_dir(struct xdlrc_tokenizer *t);

#endif /* __XDLRC_H */
