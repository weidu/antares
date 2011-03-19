#ifndef __RTREE_H
#define __RTREE_H

struct rtree_leaf_header {
	struct rtree_node *parent;
};

struct rtree_node {
	int count;
	struct rtree_node *parent;
	struct rtree_node *children[2];
};

struct rtree_node *rtree_new_root();
void rtree_add(struct rtree_node *root, void *_leaf);
void rtree_del(void *_leaf);
void *rtree_get(struct rtree_node *root, int index);
void rtree_free(struct rtree_node *root, void (*free_fn)(void *));

void rtree_print(struct rtree_node *root, int indent);

#endif /* __RTREE_H */
