/* bfs.c : 
   Perform BFS on a graph with N nodes.
   N is user configurable.
   Currently, each node has 2 children out of which at most 2 can be NULL.
   TODO: Extend to k children by adding k to struct, and modifying traversal logic.
   TODO: Handle cycles in last processing loop. Acyclic graphs assumed for now (tree);
*/

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include "zsim_hooks.h"
struct node {
    int id;
    int value1;
    int value2;
    int value3;
    int value4;
	bool isLeafNode;
    struct node *left, *right;
};

int main(int argc, char *argv[]) {

    size_t num = 15;
    printf("argc = %d\n", argc);
    if(argc == 2) {
        num = atol(argv[1]);
    }

    struct node **graph = malloc(num * sizeof(struct node *));
    if(!graph) {
        perror("Failed to allocate pointers.");
        return 1;
    }

    int i = 0;
    for(i = 0; i < num; i++) {
        graph[i] = (struct node *) malloc(sizeof(struct node));

        if(!graph[i]) {
            perror("Failed to allocate nodes for pointers.");
            return 1;
        }

        graph[i]->id = i;
    }


    for(i = 0; i < num; i++) {
        int l = (2*i) + 1;
        int r = (2*i) + 2;

        if(l < num) {
            graph[i]->left = graph[l];
						graph[i]->isLeafNode = false;
				}
        else {
            graph[i]->left = NULL;
						graph[i]->isLeafNode = true;
				}

        if(r < num) {
            graph[i]->right = graph[r];
						graph[i]->isLeafNode = false;
        }
        else {
            graph[i]->right = NULL;
						graph[i]->isLeafNode = true;
				}
    }

    for(i = 0; i < num; i++) {
        printf("id %d, addr %p, left %p, right %p\n", graph[i]->id, graph[i], graph[i]->left, graph[i]->right);
    }

		zsim_roi_begin();

    /* Graph creation complete. Nodes are randomly assigned and chained.
       Now we perform BFS on the obtained graph */
		// zsim_graph_node((uint64_t)&graph[0]);
    struct node *root = graph[0];
    struct node **queue = malloc(num * sizeof(struct node *));
    bool *visited = (bool *) malloc(num * sizeof(bool));
    memset(visited, 0, num * sizeof(bool));

    if(!queue || !visited) {
        perror("BFS structures alloc failed.");
        return 1;
    }

    queue[0] = root;
    int maxq = 1; /* Max index in queue */
    int cur = maxq - 1; /* Current index in queue being processed */

    while(cur < maxq) {
						zsim_graph_node((uint64_t)&graph[0]);

        /* printf("Processing %p\n", queue[cur]); */
        /* printf("id %d, maxq %d\n", queue[cur]->id, maxq); */
        /* printf("%p, left %p, right %p\n", queue[cur], queue[cur]->left, queue[cur]->right); */

        if(queue[cur] -> left) {
            queue[maxq] = queue[cur]->left;
            maxq++;
        }

        /* printf("%p, left %p, right %p\n", queue[cur], queue[cur]->left, queue[cur]->right); */
        if(queue[cur] -> right) {
						//zsim_graph_node(&queue[cur]);
            queue[maxq] = queue[cur]->right;
            maxq++;
        }

        visited[queue[cur]->id] = true;
        printf("%d, ", queue[cur]->id);
        cur++;

        if(maxq > num) {
            printf("WTF\n");
        }
        /* printf("cur = %d, maxq = %d\n", cur, maxq); */
    }
		zsim_roi_end();
    return 0;
}
