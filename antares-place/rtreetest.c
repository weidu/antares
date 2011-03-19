#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "rtree.h"

#define ELTS 10

int main(int argc, char *argv[])
{
	struct rtree_node *root;
	struct rtree_leaf_header *elts[ELTS];
	struct rtree_leaf_header *e;
	int i;
	
	root = rtree_new_root();
	
	for(i=0;i<ELTS;i++) {
		elts[i] = alloc_type(struct rtree_leaf_header);
		rtree_add(root, elts[i]);
	}
	rtree_print(root, 0);
	
	e = rtree_get(root, 8);
	printf("get: %p\n", e);
	
	printf("deleting:");
	for(i=0;i<ELTS/2;i++) {
		printf(" %p", elts[i]);
		rtree_del(elts[i]);
		free(elts[i]);
	}
	printf("\n");
	rtree_print(root, 0);
	
	e = rtree_get(root, 3);
	printf("get: %p\n", e);
	
	rtree_free(root, free);
	
	return 0;
}
