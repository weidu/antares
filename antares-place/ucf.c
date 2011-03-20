#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <anetlist/net.h>
#include <anetlist/entities.h>
#include <anetlist/bels.h>
#include <chip/db.h>

#include "resmgr.h"
#include "constraints.h"
#include "rtree.h"
#include "ucf.h"

static struct resmgr_bel *find_site(struct db *db, struct rtree_node *root, const char *name)
{
	int i;
	struct resmgr_bel *b;
	
	/* inefficient way of iterating the rtree, but there are few IOBs... */
	for(i=0;i<root->count;i++) {
		b = rtree_get(root, i);
		if(strcmp(db->chip.tiles[b->tile].sites[b->site_offset].name, name) == 0)
			return b;
	}
	return NULL;
}

static struct resmgr_bel *find_iob(struct resmgr *r, const char *name)
{
	struct resmgr_bel *b;
	
	b = find_site(r->db, r->free_iobm, name);
	if(b == NULL)
		b = find_site(r->db, r->free_iobs, name);
	return b;
}

void ucf_apply(struct resmgr *r, struct ucfparse *u)
{
	struct ucfparse_net *n;
	struct ucfparse_attr *attrs;
	struct anetlist_instance *inst;
	struct resmgr_bel *b;
	
	n = u->nets;
	while(n != NULL) {
		/*
		 * For now, we only support I/O constraints
		 * and we assume that the name of the UCF net
		 * is the name of the IOB.
		 * Later versions should be more subtle.
		 */
		inst = anetlist_find(r->a, n->name);
		if(inst == NULL) {
			fprintf(stderr, "Instance '%s' from UCF not found in netlist\n", n->name);
			exit(EXIT_FAILURE);
		}
		if((inst->e != &anetlist_bels[ANETLIST_BEL_IOBM]) && (inst->e != &anetlist_bels[ANETLIST_BEL_IOBS])) {
			fprintf(stderr, "Only I/O constraints are supported now\n");
			exit(EXIT_FAILURE);
		}
		attrs = n->attrs;
		while(attrs != NULL) {
			switch(attrs->attr) {
				case UCF_ATTR_LOC:
				 	b = find_iob(r, attrs->value);
				 	if(b == NULL) {
				 		fprintf(stderr, "Failed to find IOB site %s\n", attrs->value);
				 		exit(EXIT_FAILURE);
				 	}
					constraints_lock(inst, b);
					break;
				case UCF_ATTR_IOSTANDARD:
					anetlist_set_attribute(inst, 
						entity_find_attr(inst->e, "ISTANDARD"),
						attrs->value);
					anetlist_set_attribute(inst, 
						entity_find_attr(inst->e, "OSTANDARD"),
						attrs->value);
					break;
				case UCF_ATTR_SLEW:
					anetlist_set_attribute(inst, 
						entity_find_attr(inst->e, "SLEW_RATE"),
						attrs->value);
					break;
				case UCF_ATTR_DRIVE:
					anetlist_set_attribute(inst, 
						entity_find_attr(inst->e, "DRIVE_STRENGTH"),
						attrs->value);
					break;
				default:
					fprintf(stderr, "Warning: unsupported UCF attribute\n");
					break;
			}
			attrs = attrs->next;
		}
		n = n->next;
	}
}
