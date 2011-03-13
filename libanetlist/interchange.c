#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <anetlist/entities.h>
#include <anetlist/net.h>
#include <anetlist/interchange.h>

enum {
	CMD_NONE,
	CMD_MODULE,
	CMD_PART,
	CMD_INPUT,
	CMD_OUTPUT,
	CMD_INST,
	CMD_NET
};

static int str_to_cmd(const char *str)
{
	if(str == NULL) return CMD_NONE;
	if(strcmp(str, "-") == 0) return CMD_NONE;
	if(strcmp(str, "module") == 0) return CMD_MODULE;
	if(strcmp(str, "part") == 0) return CMD_PART;
	if(strcmp(str, "input") == 0) return CMD_INPUT;
	if(strcmp(str, "output") == 0) return CMD_OUTPUT;
	if(strcmp(str, "inst") == 0) return CMD_INST;
	if(strcmp(str, "net") == 0) return CMD_NET;
	fprintf(stderr, "Invalid command: %s\n", str);
	exit(EXIT_FAILURE);
	return 0;
}

static const char delims[] = " \t\n";

static char *get_token(char **saveptr)
{
	char *token;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of line\n");
		exit(EXIT_FAILURE);
	}
	return token;
}

static void parse_module(struct anetlist *a, char **saveptr)
{
	anetlist_set_module_name(a, get_token(saveptr));
}

static void parse_part(struct anetlist *a, char **saveptr)
{
	anetlist_set_part_name(a, get_token(saveptr));
}

static void parse_inst(struct anetlist *a, char **saveptr, int command, anetlist_find_entity_c find_entity_c)
{
	struct anetlist_entity *e;
	char *uid;
	char *token;
	struct anetlist_instance *inst;
	
	uid = get_token(saveptr);
	if(command == CMD_INPUT)
		e = &entity_input_port;
	else if(command == CMD_OUTPUT)
		e = &entity_output_port;
	else {
		token = get_token(saveptr);
		e = find_entity_c(token);
		if(e == NULL) {
			fprintf(stderr, "Unrecognized entity: %s\n", token);
			exit(EXIT_FAILURE);
		}
	}
	
	inst = anetlist_instantiate(a, uid, e);
	while((token = strtok_r(NULL, delims, saveptr))) {
		if(strcmp(token, "attr") == 0) {
			char *attr_s;
			int attr;
			char *value;
			
			attr_s = get_token(saveptr);
			attr = entity_find_attr(e, attr_s);
			if(attr == -1) {
				fprintf(stderr, "Unexpected entity attribute: %s\n", attr_s);
				exit(EXIT_FAILURE);
			}
			value = get_token(saveptr);
			anetlist_set_attribute(inst, attr, value);
		} else {
			fprintf(stderr, "Unexpected entity parameter: %s\n", token);
			exit(EXIT_FAILURE);
		}
	}
}

static void parse_net(struct anetlist *a, char **saveptr)
{
	char *s;
	struct anetlist_instance *start_inst;
	int start_pin;
	char *token;
	struct anetlist_instance *end_inst;
	int end_pin;
	
	s = get_token(saveptr);
	start_inst = anetlist_find(a, s);
	if(start_inst == NULL) {
		fprintf(stderr, "Source instance not found: %s\n", s);
		exit(EXIT_FAILURE);
	}
	s = get_token(saveptr);
	start_pin = entity_find_pin(start_inst->e, 1, s);
	if(start_pin == -1) {
		fprintf(stderr, "Output pin not found: %s\n", s);
		exit(EXIT_FAILURE);
	}
	while((token = strtok_r(NULL, delims, saveptr))) {
		if(strcmp(token, "end") == 0) {
			s = get_token(saveptr);
			end_inst = anetlist_find(a, s);
			if(end_inst == NULL) {
				fprintf(stderr, "Sink instance not found: %s\n", s);
				exit(EXIT_FAILURE);
			}
			s = get_token(saveptr);
			end_pin = entity_find_pin(end_inst->e, 0, s);
			if(end_pin == -1) {
				fprintf(stderr, "Input pin not found: %s\n", s);
				exit(EXIT_FAILURE);
			}
			anetlist_connect(start_inst, start_pin, end_inst, end_pin);
		} else {
			fprintf(stderr, "Unexpected net parameter: %s\n", token);
			exit(EXIT_FAILURE);
		}
	}
}

static void parse_line(struct anetlist *a, char *line, anetlist_find_entity_c find_entity_c)
{
	char *saveptr;
	char *str;
	int command;

	str = strtok_r(line, delims, &saveptr);
	command = str_to_cmd(str);
	switch(command) {
		case CMD_NONE:
			return;
		case CMD_MODULE:
			parse_module(a, &saveptr);
			break;
		case CMD_PART:
			parse_part(a, &saveptr);
			break;
		case CMD_INPUT:
		case CMD_OUTPUT:
		case CMD_INST:
			parse_inst(a, &saveptr, command, find_entity_c);
			break;
		case CMD_NET:
			parse_net(a, &saveptr);
			break;
	}
	str = strtok_r(NULL, delims, &saveptr);
	if(str != NULL) {
		fprintf(stderr, "Expected end of line, got token: %s\n", str);
		exit(EXIT_FAILURE);
	}
}

struct anetlist *anetlist_parse_fd(FILE *fd, anetlist_find_entity_c find_entity_c)
{
	struct anetlist *a;
	char *line;
	size_t linesize;
	int r;
	
	a = anetlist_new();
	line = NULL;
	linesize = 0;
	while(1) {
		r = getline(&line, &linesize, fd);
		if(r == -1) {
			assert(feof(fd));
			break;
		}
		parse_line(a, line, find_entity_c);
	}
	free(line);
	
	return a;
}

static void write_ports(struct anetlist *a, FILE *fd)
{
	struct anetlist_instance *inst;
	int i;
	
	inst = a->head;
	while(inst != NULL) {
		if((inst->e->type == ANETLIST_ENTITY_PORT_OUT) || (inst->e->type == ANETLIST_ENTITY_PORT_IN)) {
			if(inst->e->type == ANETLIST_ENTITY_PORT_OUT)
				fprintf(fd, "output %s", inst->uid);
			else
				fprintf(fd, "input %s", inst->uid);
			for(i=0;i<inst->e->n_attributes;i++)
				fprintf(fd, " attr %s %s",
					inst->e->attribute_names[i],
					inst->attributes[i]);
			fprintf(fd, "\n");
		}
		inst = inst->next;
	}
}

static void write_instances(struct anetlist *a, FILE *fd)
{
	struct anetlist_instance *inst;
	int i;
	
	inst = a->head;
	while(inst != NULL) {
		if(inst->e->type == ANETLIST_ENTITY_INTERNAL) {
			fprintf(fd, "inst %s %s", inst->uid, inst->e->name);
			for(i=0;i<inst->e->n_attributes;i++)
				fprintf(fd, " attr %s %s",
					inst->e->attribute_names[i],
					inst->attributes[i]);
			fprintf(fd, "\n");
		}
		inst = inst->next;
	}
}

static void write_nets(struct anetlist *a, FILE *fd)
{
	struct anetlist_instance *inst;
	struct anetlist_endpoint *ep;
	int i;
	
	inst = a->head;
	while(inst != NULL) {
		for(i=0;i<inst->e->n_outputs;i++) {
			ep = inst->outputs[i];
			if(ep != NULL) {
				fprintf(fd, "net %s %s", inst->uid, inst->e->output_names[i]);
				while(ep != NULL) {
					fprintf(fd, " end %s %s", ep->inst->uid, ep->inst->e->input_names[ep->pin]);
					ep = ep->next;
				}
				fprintf(fd, "\n");
			}
		}
		inst = inst->next;
	}
}

void anetlist_write_fd(struct anetlist *a, FILE *fd)
{
	fprintf(fd, "module %s\n", a->module_name);
	fprintf(fd, "part %s\n", a->part_name);
	write_ports(a, fd);
	write_instances(a, fd);
	write_nets(a, fd);
}

struct anetlist *anetlist_parse_file(const char *filename, anetlist_find_entity_c find_entity_c)
{
	FILE *fd;
	struct anetlist *a;

	fd = fopen(filename, "r");
	if(fd == NULL) {
		perror("anetlist_parse_file");
		exit(EXIT_FAILURE);
	}
	a = anetlist_parse_fd(fd, find_entity_c);
	fclose(fd);
	return a;
}

void anetlist_write_file(struct anetlist *a, const char *filename)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	if(fd == NULL) {
		perror("anetlist_write_file");
		exit(EXIT_FAILURE);
	}
	anetlist_write_fd(a, fd);
	r = fclose(fd);
	if(r != 0) {
		perror("anetlist_write_file");
		exit(EXIT_FAILURE);
	}
}
