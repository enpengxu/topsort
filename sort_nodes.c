#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h>

#define NODE_COLOR_WHITE 0
#define NODE_COLOR_BLACK 1

struct node {
	int src;
	int dst;

	unsigned order;
	unsigned color;
	struct node * dep;
	struct node * next;
	struct node * prev;
};

struct node_list {
	unsigned num;
	struct node  head;
};

#define list_init(list)				   \
	(list)->head.next = &(list)->head;	   \
	(list)->head.prev = &(list)->head

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

static inline struct node *
node_find_first_dep(struct node_list * list,
			  int dst,
			  struct node * start)
{
	struct node * node = start;
	for_each_node_prev(node, list) {
		if (node->dst == dst)
			return node;
	}
	return NULL;
}


/* check if node is depend on target */
static inline int
node_is_depend_on(struct node_list * list,
				 struct node * node,
				 struct node * target)
{
	struct node * dep_src;
	struct node * dep_dst;
	if (node->order < target->order)
		return 0;
	if (node->dst == target->dst ||
		node->src == target->dst)
		return 1;
	dep_src  = node_find_first_dep(list, node->src, node->prev);
	dep_dst  = node_find_first_dep(list, node->dst, node->prev);

	if (dep_src && node_is_depend_on(list, dep_src, target)){
		return 1;
	}
	if (dep_dst && node_is_depend_on(list, dep_dst, target)){
		return 1;
	}
	return 0;
}

static inline int
node_is_mergable(struct node_list * list,
				 struct node * n0,
				 struct node * n1)
{
	struct node * node, *dep;
	for_each_node_between_prev(node, n0, n1) {
		if (node_is_depend_on(list, node, n0))
			return 0;
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
node_list_sort(struct node_list * list, struct node **ptr_list)
{
	struct node * n0, *n1, * tmp;
	assert(list && list->head.next && ptr_list);

	unsigned num = list->num - 1;
	n1 = last_node(list);
	for_each_node_prev(n1, list) {
		if (n1->color == NODE_COLOR_BLACK)
			continue;

		ptr_list[num--] = n1;
		n0 = n1;
		tmp = n1;
		while(n0 = node_find_first_dep(list, n0->dst, n0->prev)) {
			if (node_is_mergable(list, n0, tmp)) {
				//node_merge(list, n0, n1);
				n0->color = NODE_COLOR_BLACK;
				ptr_list[num--] = n0;
				tmp = n0;
			}
		}
		n1->color = NODE_COLOR_BLACK;
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
	list_init(list);
	list->num = num;
	for (p = &list->head, i=0; i<num; i++) {
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

static void
node_ptr_list_dump(struct node ** ptr_list, unsigned num, const char * str)
{
	unsigned i;
	fprintf(stderr, "%s \n", str);
	for (i=0; i<num; i++) {
		fprintf(stderr, "node[%2d] : src/dst { %3d / %3d } \n",
				i, ptr_list[i]->src, ptr_list[i]->dst);
	}
	fprintf(stderr, "\n");
}

static struct dst_src test1[]=  {
	{ .src = 101, .dst = 1 },
	{ .src = 102, .dst = 2 },
	{ .src = 104, .dst = 1 }
};

static struct dst_src test2[]=  {
	{ .src = 2, .dst = 1 },
	{ .src = 1, .dst = 2 },
	{ .src = 2, .dst = 1 }
};

static struct dst_src test3[]=  {
	{ .src = 100, .dst = 4 },
	{ .src = 101, .dst = 1 },
	{ .src =   4, .dst = 3 },
	{ .src =   3, .dst = 2 },
	{ .src =   2, .dst = 1 },
	{ .src =   2, .dst = 1 },
};

static struct dst_src test4[]=  {
	{ .src = 100, .dst = 4 },
	{ .src = 101, .dst = 1 },
	{ .src =   1, .dst = 2 }, /* it should stop merging */
	{ .src =   4, .dst = 3 },
	{ .src =   3, .dst = 2 },
	{ .src =   2, .dst = 1 }
};


static int
do_test (struct dst_src * test, int num)
{
	struct node_list list;
	struct node ** ptr_list;
	node_list_create(&list, num, test);
	node_list_dump(&list, "before ");

	ptr_list = calloc(list.num, sizeof(*ptr_list));
	node_list_sort(&list, ptr_list);
	node_ptr_list_dump(ptr_list, num, "after sort");
	free(ptr_list);
}

int main(int argc, char **argv)
{
	do_test(test1, sizeof(test1)/sizeof(test1[0]));
	do_test(test2, sizeof(test2)/sizeof(test2[0]));
	do_test(test3, sizeof(test3)/sizeof(test3[0]));
	do_test(test4, sizeof(test4)/sizeof(test4[0]));
	return 0;
}
