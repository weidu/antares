#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <util.h>

#include "xdlrc.h"

static int xdlrc_is_eol(char c)
{
	return (c == '\n') || (c == 0);
}

struct xdlrc_tokenizer *xdlrc_create_tokenizer(const char *filename)
{
	struct xdlrc_tokenizer *t;
	
	t = alloc_type(struct xdlrc_tokenizer);
	t->fd = fopen(filename, "r");
	if(t->fd == NULL) {
		perror("xdlrc_init_tokenizer");
		exit(EXIT_FAILURE);
	}
	t->line = NULL;
	t->start = NULL;
	return t;
}

void xdlrc_free_tokenizer(struct xdlrc_tokenizer *t)
{
	free(t->line);
	fclose(t->fd);
	free(t);
}

static int xdlrc_newline(struct xdlrc_tokenizer *t)
{
	int r;

	do {
		r = getline(&t->line, &t->n, t->fd);
		if(r == -1)
			return 0;
	} while(*t->line == '#');
	t->start = t->line;
	return 1;
}

char *xdlrc_get_token(struct xdlrc_tokenizer *t)
{
	char *end;
	char *ret;
	
	if(t->start == NULL) {
		if(!xdlrc_newline(t)) return NULL;
	}
	while(1) {
		while(isspace(*t->start))
			t->start++;
		if(!xdlrc_is_eol(*t->start))
			break;
		if(!xdlrc_newline(t)) return NULL;
	}
	if((*t->start == '(') || (*t->start == ')')) {
		ret = alloc_size(2);
		ret[0] = *t->start;
		ret[1] = 0;
		t->start++;
	} else {
		end = t->start;
		while(!isspace(*end) && !xdlrc_is_eol(*end) && (*end != '(') && (*end != ')'))
			end++;
		ret = alloc_size(end-t->start+1);
		memcpy(ret, t->start, end-t->start);
		ret[end-t->start] = 0;
		t->start = end;
	}
	return ret;
}

char *xdlrc_get_token_noeof(struct xdlrc_tokenizer *t)
{
	char *s;
	
	s = xdlrc_get_token(t);
	if(s == NULL) {
		fprintf(stderr, "Unexpected EOF\n");
		exit(EXIT_FAILURE);
	}
	return s;
}

void xdlrc_get_token_par(struct xdlrc_tokenizer *t, int open)
{
	char *s;
	
	s = xdlrc_get_token_noeof(t);
	if(strcmp(s, open ? "(" : ")") != 0) {
		fprintf(stderr, "Expected parenthese, got: '%s'\n", s);
		exit(EXIT_FAILURE);
	}
	free(s);
}

int xdlrc_get_token_int(struct xdlrc_tokenizer *t)
{
	char *s;
	char *c;
	int r;
	
	s = xdlrc_get_token_noeof(t);
	r = strtoul(s, &c, 0);
	if(*c != 0) {
		fprintf(stderr, "Expected integer, got: '%s'\n", s);
		exit(EXIT_FAILURE);
	}
	free(s);
	return r;
}

int xdlrc_get_token_dir(struct xdlrc_tokenizer *t)
{
	char *s;
	int r;
	
	s = xdlrc_get_token(t);
	if(strcmp(s, "==") == 0) r = XDLRC_BIDIR_UNBUFFERED;
	else if(strcmp(s, "=>") == 0) r = XDLRC_BIDIR_BUFFERED1;
	else if(strcmp(s, "=-") == 0) r = XDLRC_BIDIR_BUFFERED2;
	else if(strcmp(s, "->") == 0) r = XDLRC_DIR_BUFFERED;
	else {
		fprintf(stderr, "Expected direction, got: '%s'\n", s);
		exit(EXIT_FAILURE);
	}
	free(s);
	return r;
}

void xdlrc_close_parenthese(struct xdlrc_tokenizer *t)
{
	int counter;
	char *s;
	
	counter = 0;
	while(counter != -1) {
		s = xdlrc_get_token_noeof(t);
		if(strcmp(s, "(") == 0) counter++;
		else if(strcmp(s, ")") == 0) counter--;
		free(s);
	}
}
