#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <ucfparse/ucfparse.h>

static struct ucfparse *ucfparse_new()
{
	struct ucfparse *u;
	
	u = alloc_type(struct ucfparse);
	u->nets = NULL;
	return u;
}

static struct ucfparse_net *ucfparse_get_net(struct ucfparse *u, const char *name)
{
	struct ucfparse_net *n;
	
	n = u->nets;
	while(n != NULL) {
		if(strcmp(n->name, name) == 0) return n;
		n = n->next;
	}
	
	n = alloc_type(struct ucfparse_net);
	n->name = stralloc(name);
	n->attrs = NULL;
	n->next = u->nets;
	u->nets = n;
	
	return n;
}

static int str_to_attr(const char *attr)
{
	if(strcmp(attr, "LOC") == 0) return UCF_ATTR_LOC;
	if(strcmp(attr, "IOSTANDARD") == 0) return UCF_ATTR_IOSTANDARD;
	if(strcmp(attr, "SLEW") == 0) return UCF_ATTR_SLEW;
	if(strcmp(attr, "DRIVE") == 0) return UCF_ATTR_DRIVE;
	return -1;
}

static void ucfparse_set_attr(struct ucfparse_net *net, const char *attr, const char *value)
{
	int iattr;
	struct ucfparse_attr *a;
	
	iattr = str_to_attr(attr);
	if(iattr == -1) {
		printf("Ignoring unknown attribute '%s' in UCF\n", attr);
		return;
	}
	
	a = alloc_type(struct ucfparse_attr);
	a->attr = iattr;
	a->value = stralloc(value);
	a->next = net->attrs;
	net->attrs = a;
}

static const char delims[] = " \t\n;=";

enum {
	TID_NET,
	TID_HASH,
	TID_OR,
	TID_UNKNOWN,
	TID_NULL
};

static int str_to_tid(const char *str)
{
	if(str == NULL) return TID_NULL;
	if(strcmp(str, "NET") == 0) return TID_NET;
	if(strcmp(str, "#") == 0) return TID_HASH;
	if(strcmp(str, "|") == 0) return TID_OR;
	return TID_UNKNOWN;
}

static int tid_invalid_name(int tid)
{
	return (tid == TID_HASH) || (tid == TID_OR);
}

static char *dequote(char *str)
{
	char *c;
	
	if(*str != '"')
		return str;
	str++;
	c = str+strlen(str)-1;
	if((*c != '"') || (c == str)) {
		fprintf(stderr, "Missing closing quote\n");
		exit(EXIT_FAILURE);
	}
	*c = 0;
	return str;
}

enum {
	STATE_START,
	STATE_NETNAME,
	STATE_NET,
	STATE_ATTR_VALUE,
	STATE_ATTR_NEXT
};

static void parse_line(struct ucfparse *u, char *line)
{
	char *saveptr;
	char *str;
	int tid;
	int state;
	struct ucfparse_net *net;
	char *attr;

	state = STATE_START;
	while(1) {
		str = strtok_r(line, delims, &saveptr);
		line = NULL;
		tid = str_to_tid(str);
		switch(state) {
			case STATE_START:
				switch(tid) {
					case TID_NULL:
					case TID_HASH:
						return;
					case TID_NET:
						state = STATE_NETNAME;
						break;
					default:
						fprintf(stderr, "Invalid start of line: %s\n", str);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			case STATE_NETNAME:
				if(tid_invalid_name(tid)) {
					fprintf(stderr, "Invalid net name: %s\n", str);
					exit(EXIT_FAILURE);
				}
				net = ucfparse_get_net(u, dequote(str));
				state = STATE_NET;
				break;
			case STATE_NET:
				switch(tid) {
					case TID_NULL:
					case TID_HASH:
						return;
					case TID_NET:
						state = STATE_NETNAME;
						break;
					default:
						if(tid_invalid_name(tid)) {
							fprintf(stderr, "Invalid net name: %s\n", str);
							exit(EXIT_FAILURE);
						}
						attr = stralloc(dequote(str));
						state = STATE_ATTR_VALUE;
						break;
				}
				break;
			case STATE_ATTR_VALUE:
				if(tid_invalid_name(tid)) {
					fprintf(stderr, "Invalid attribute value: %s\n", str);
					exit(EXIT_FAILURE);
				}
				ucfparse_set_attr(net, attr, dequote(str));
				free(attr);
				state = STATE_ATTR_NEXT;
				break;
			case STATE_ATTR_NEXT:
				switch(tid) {
					case TID_NULL:
					case TID_HASH:
						return;
					case TID_NET:
						state = STATE_NETNAME;
						break;
					case TID_OR:
						state = STATE_NET;
						break;
					default:
						fprintf(stderr, "Unexpected token after attribute value: %s\n", str);
						exit(EXIT_FAILURE);
						break;
				}
				break;
			default:
				assert(0);
				break;
		}
	}
}

struct ucfparse *ucfparse_fd(FILE *fd)
{
	struct ucfparse *u;
	int r;
	char *line;
	size_t linesize;
	
	u = ucfparse_new();
	
	line = NULL;
	linesize = 0;
	while(1) {
		r = getline(&line, &linesize, fd);
		if(r == -1) {
			assert(feof(fd));
			break;
		}
		parse_line(u, line);
	}
	free(line);
	
	return u;
}

static void free_attr(struct ucfparse_attr *a)
{
	free(a->value);
	free(a);
}

static void free_net(struct ucfparse_net *n)
{
	struct ucfparse_attr *a1, *a2;
	
	free(n->name);
	a1 = n->attrs;
	while(a1 != NULL) {
		a2 = a1->next;
		free_attr(a1);
		a1 = a2;
	}
	free(n);
}

void ucfparse_free(struct ucfparse *u)
{
	struct ucfparse_net *n1, *n2;
	
	n1 = u->nets;
	while(n1 != NULL) {
		n2 = n1->next;
		free_net(n1);
		n1 = n2;
	}
	free(u);
}

struct ucfparse *ucfparse_file(const char *filename)
{
	FILE *fd;
	struct ucfparse *u;

	fd = fopen(filename, "r");
	if(fd == NULL) {
		perror("ucf_read_file");
		exit(EXIT_FAILURE);
	}
	u = ucfparse_fd(fd);
	fclose(fd);
	return u;
}
