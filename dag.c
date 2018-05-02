#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#define DAG_COLOR_WHITE  0
#define DAG_COLOR_GRAY   1
#define DAG_COLOR_BLACK  2

struct dag_node {
	const char * name;

	int enter_time;
	int leave_time;
	int color;
	int adj_num;
	struct dag_node ** adj;
	struct dag_node  * sorted_next;
};

struct dag_graph {
	int node_num;
	struct dag_node ** nodes;
	struct dag_node  * sorted;
};

struct node_source {
	const char  * name;
	const char  ** deps;
};

static const char * deps_shirt[]  = { "tie",   "belt",  NULL };
static const char * deps_under[]  = { "pants", "shoes", NULL };
static const char * deps_socks[]  = { "shoes",  NULL };
static const char * deps_pants[]  = { "belt",  "shoes", NULL };
static const char * deps_belt []  = { "jacket", NULL };
static const char * deps_tie  []  = { "jacket", NULL };

static struct node_source source[] = {
	{ "shirt",       deps_shirt },
	{ "watch",       NULL       },
	{ "undershorts", deps_under },
	{ "socks",       deps_socks },
	{ "pants",       deps_pants },
	{ "belt",        deps_belt  },
	{ "tie",         deps_tie   },
	{ "jacket",      NULL       },
	{ "shoes",       NULL       },
};

struct dag_node *
find_node(struct dag_graph * g, const char * name)
{
	int i;
	for (i=0; i< g->node_num; i++) {
		if (strcmp(g->nodes[i]->name, name) == 0) {
			return g->nodes[i];
		}
	}
	assert(0 && "no node found !");
	return NULL;
}

static int
dag_graph_build(struct dag_graph * g)
{
	int i, j, n = sizeof(source)/sizeof(source[0]);
	g->node_num = n;
	g->nodes = malloc(n*sizeof(struct dag_node *));
	g->sorted = NULL;
	for (i=0; i<n; i++) {
		g->nodes[i] = calloc(1, sizeof(struct dag_node));
		g->nodes[i]->name = source[i].name;
		g->nodes[i]->adj_num = 0;
		while (source[i].deps &&
			   source[i].deps[g->nodes[i]->adj_num]) {
			g->nodes[i]->adj_num ++;
		}
		if (g->nodes[i]->adj_num) {
			g->nodes[i]->adj = calloc(g->nodes[i]->adj_num,
					sizeof(struct dag_node *));
		}
	}
	for (i=0; i<n; i++) {
		for (j=0; j<g->nodes[i]->adj_num; j++) {
			assert(source[i].deps[j]);
			g->nodes[i]->adj[j] = find_node(g, source[i].deps[j]);
		}
	}
}

static void
dag_graph_dfs_start(struct dag_graph * g,
		struct dag_node * node, int * time)
{
	int i;
	assert (node->color == DAG_COLOR_WHITE);

	(*time) ++;
	node->enter_time = *time;
	node->color = DAG_COLOR_GRAY;

	for (i=0; i < node->adj_num; i++) {
		if (node->adj[i]->color == DAG_COLOR_WHITE)
			dag_graph_dfs_start(g, node->adj[i], time);
	}
	(*time) ++;
	node->leave_time = *time;

	node->color = DAG_COLOR_BLACK;
	node->sorted_next = g->sorted;
	g->sorted = node;
}


static void
dag_graph_dfs(struct dag_graph * g)
{
	int i, time = 0;
	for (i=0; i<g->node_num; i++) {
		struct dag_node * node = g->nodes[i];
		switch(node->color) {
		case DAG_COLOR_WHITE:
			dag_graph_dfs_start(g, node, &time);
			break;
		case DAG_COLOR_GRAY:
			assert(0 && "node is gray color impossible !");
			break;
		case DAG_COLOR_BLACK:
		default:
			break;
		}
	}
}

static void
dag_graph_dump(struct dag_graph * g)
{
	int i, j;
	for (i=0; i<g->node_num; i++) {
		fprintf(stderr, "\n[node %2d] name %s, deps %d: ",
				i, g->nodes[i]->name,
				g->nodes[i]->adj_num);
		for (j=0; j<g->nodes[i]->adj_num; j++) {
			fprintf(stderr, "%s,", g->nodes[i]->adj[j]->name);
		}
	}
	fprintf(stderr, "\n");
}

static void
dag_graph_sorted_dump(struct dag_graph * g)
{
	int n=0;
	struct dag_node * node = g->sorted;
	fprintf(stderr, "topological sorting \n");
	while(node) {
		fprintf(stderr, "sorted node [%d] name %s %d/%d \n",
				n++, node->name, node->enter_time, node->leave_time);
		node = node->sorted_next;
	}
	fprintf(stderr, "\n");
}


static void
dag_test()
{
	int i;
	struct dag_graph g;
	dag_graph_build(&g);
	dag_graph_dump(&g);

	dag_graph_dfs(&g);
	dag_graph_sorted_dump(&g);
}


int main()
{
	dag_test();
	return 0;
}

