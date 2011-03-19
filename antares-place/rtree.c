#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <util.h>

#include "rtree.h"

struct rtree_node *rtree_new_root()
{
	struct rtree_node *root;
	
	root = alloc_type(struct rtree_node);
	root->parent = NULL;
	root->count = 0;
	return root;
}

void rtree_add(struct rtree_node *root, void *_leaf)
{
	struct rtree_node *current = root;
	struct rtree_leaf_header *leaf = _leaf;
	struct rtree_leaf_header *leaf2;
	struct rtree_node *ins;
	
	if(root->count == 0) {
		root->count = 1;
		root->children[0] = (void *)leaf;
		leaf->parent = root;
		return;
	}
	
	while(current->count > 1) {
		if(current->children[0]->count < current->children[1]->count)
			current = current->children[0];
		else
			current = current->children[1];
	}
	assert(current->count > 0);
	
	leaf2 = (struct rtree_leaf_header *)current->children[0];
	ins = alloc_type(struct rtree_node);
	ins->count = 1;
	ins->parent = current;
	ins->children[0] = (void *)leaf2;
	current->children[0] = ins;
	leaf2->parent = ins;

	ins = alloc_type(struct rtree_node);
	ins->count = 1;
	ins->parent = current;
	ins->children[0] = (void *)leaf;
	current->children[1] = (struct rtree_node *)ins;
	leaf->parent = ins;
	
	while(current != NULL) {
		current->count++;
		current = current->parent;
	}
}

void rtree_del(void *_leaf)
{
	struct rtree_leaf_header *leaf = _leaf;
	struct rtree_node *lparent, *pparent;
	struct rtree_node *otherchild;
	
	lparent = leaf->parent;
	while(lparent != NULL) {
		lparent->count--;
		lparent = lparent->parent;
	}

	lparent = leaf->parent;
	pparent = lparent->parent;
	if(pparent == NULL)
		return; /* root */
	free(lparent);
	if(pparent->children[0] == lparent)
		otherchild = pparent->children[1];
	else
		otherchild = pparent->children[0];
	assert(pparent->count == otherchild->count);
	pparent->children[0] = otherchild->children[0];
	if(pparent->count == 1)
		((struct rtree_leaf_header *)(pparent->children[0]))->parent = pparent;
	else
		pparent->children[0]->parent = pparent;
	if(pparent->count > 1) {
		pparent->children[1] = otherchild->children[1];
		pparent->children[1]->parent = pparent;
	}
	free(otherchild);
}

void *rtree_get(struct rtree_node *root, int index)
{
	if((index < 0) || (index >= root->count))
		return NULL;
	if(root->count == 1)
		return root->children[0];
	if(index >= root->children[0]->count)
		return rtree_get(root->children[1], index-root->children[0]->count);
	else
		return rtree_get(root->children[0], index);
}

void rtree_free(struct rtree_node *root, void (*free_fn)(void *))
{
	if(root->count == 1)
		free_fn(root->children[0]);
	if(root->count >= 2) {
		rtree_free(root->children[0], free_fn);
		rtree_free(root->children[1], free_fn);
	}
	free(root);
}

void rtree_print(struct rtree_node *root, int indent)
{
	int i;
	
	for(i=0;i<indent;i++)
		printf("  ");
	printf("%p: parent=%p count=%d", root, root->parent, root->count);
	if(root->count == 1)
		printf("[%p]", root->children[0]);
	printf("\n");
	if(root->count >= 2) {
		rtree_print(root->children[0], indent+1);
		rtree_print(root->children[1], indent+1);
	}
}
