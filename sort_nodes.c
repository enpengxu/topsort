#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h>

struct node {
	int src;
	int dst;

	unsigned order;
	struct node * dep;
	struct node * next;
	struct node * prev;
	struct node * dst_next;  /* with same dst */
};

struct node_list {
	struct node  head;
};

#define first_node(list)  (list)->head.next
#define last_node(list)   (list)->head.prev

#define for_each_node_next(node, list)				\
	for (; (node) != &(list)->head; (node) = (node)->next)

#define for_each_node_prev(node, list)	\
	for (; (node) != &(list)->head; (node) = (node)->prev)

#define for_each_node_between_prev(node, n0, n1)	\
	for (node=(n1)->prev; node != (n0); node = node->prev)

#define remove_node(node)							\
	(node)->next->prev = (node)->prev;				\
	(node)->prev->next = (node)->next

/* insert node before n*/
#define insert_node_before(node, n)	\
	(node)->next = (n);				\
	(node)->prev = (n)->prev;			\
	(n)->prev->next = (node);			\
	(n)->prev = (node)

/* insert node after n*/
#define insert_node_after(node, n)	\
	(node)->next = (n)->next;			\
	(node)->prev = (n);				\
	(n)->next->prev = (node);			\
	(n)->next = (node)


static void
node_list_init(struct node_list * list)
{
	unsigned order = 1;
	struct node * node;

	node = first_node(list);
	for_each_node_next(node, list) {
		node->order = order ++;
		node->dep  = NULL;
		node->dst_next = NULL;
	}
	/* init dep */
	node = last_node(list);
	for_each_node_prev(node, list) {
		struct node * prev = node->prev;
		for_each_node_prev(prev, list) {
			if (prev->dst == node->src) {
				node->dep = prev;
				break;
			}
		}
	}
}

static inline int
node_is_mergable(struct node_list * list,
				 struct node * n0,
				 struct node * n1)
{
	struct node * node, *dep;
	for_each_node_between_prev(node, n0, n1) {
		dep = node;
		while(dep) {
			if (dep->dep && dep->dep->order <= n0->order) {
				return 0;
			}
			dep = dep->dep;
		}
	}
	return 1;
}
/**
 * merge n0 & n1 if possible
 * n0->dst == n1->dst
 */
static int
node_merge(struct node_list * list,
		   struct node * n0,
		   struct node * n1)
{
	int i, reorder = 0;
	struct node * node, * bottom = n0;

	assert(n0 && n1 && n0->dst == n1->dst);

	node = n1->dep;
	while(node && node->order < n0->order) {
		assert(node->order < n0->order);
		remove_node(node);
		insert_node_before(node, bottom);
		bottom = node;
		node = node->dep;
		reorder = 1;
	}
	remove_node(n1);
	insert_node_after(n1, n0);
	if (1 || reorder) {
		/* re-assign orders */
		/* TODO. no need to go through all the nodes */
		int order = 1;
		node = first_node(list);
		for_each_node_next(node, list) {
			node->order = order ++;
		}
	}
	return 0;
}

static void
node_list_sort(struct node_list * list)
{
	struct node * n0, *n1, * tmp;
	assert(list && list->head.next);

	for (n1 = last_node(list); n1 != &(list)->head;) {
		tmp = n1->prev;
		n0 = n1->prev;
		while(n0 != &list->head && n0->dst != n1->dst) {
			n0 = n0->prev;
		}
		if (n0 != &list->head && n0->dst == n0->dst &&
			n0->next != n1) {
			if (node_is_mergable(list, n0, n1)) {
				node_merge(list, n0, n1);
			}
		}
		n1 = tmp;
	}
}

struct dst_src {
	int src;
	int dst;
};

static int
node_list_create(struct node_list * list,
				 int num,
				 struct dst_src * ds)
{
	int i;
	struct node * node, *p;
	list->head.next = &list->head;
	list->head.prev = &list->head;

	for (p= &list->head, i=0; i<num; i++) {
		node = calloc(1, sizeof(*node));
		node->src = ds[i].src;
		node->dst = ds[i].dst;

		p->next = node;
		node->prev = p;
		node->next = &list->head;
		list->head.prev = node;
		p = node;
	}

	node_list_init(list);
	return 0;
}

static void
node_list_dump(struct node_list * list, const char * str)
{
	int i=0;
	struct node * node = first_node(list);
	fprintf(stderr, "%s \n", str);
	for_each_node_next(node, list) {
		fprintf(stderr, "node[%2d] : src/dst { %3d / %3d } \n",
				i++, node->src, node->dst);
	}
}

static struct dst_src test1[]=  {
	{ .src = 101, .dst = 1 },
	{ .src = 102, .dst = 2 },
	{ .src = 103, .dst = 2 },
	{ .src = 104, .dst = 1 }
};

static int
do_test1 ()
{
	struct node_list list;
	int num = sizeof(test1)/sizeof(test1[0]);

	node_list_create(&list, num, test1);
	node_list_dump(&list, "before ");
	node_list_sort(&list);
	node_list_dump(&list, "after sort");
}

int main(int argc, char **argv)
{
	do_test1();
	return 0;
}
