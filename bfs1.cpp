#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <queue>
#include "zsim_hooks.h"

using namespace std;

struct node{
	int isleafnode;	
	int value;
	struct node *left,*right;
};

struct node* buildnode(int i)
{
	struct node *root = (struct node*)malloc(sizeof(struct node));
	root->value = i;
	root->left = NULL;
	root->right = NULL;
	root->isleafnode = 1;
	return root;

}


struct node* buildgraph(int n)
{
	
	queue<node*> q1;
	int counter = 0;
	
	struct node *root = buildnode(0);
	counter++;

	q1.push(root);
	
	while(counter<n) {
		
	
		struct node* curr = q1.front();
		q1.pop();
		curr->isleafnode = 0;
		
		struct node* leftnode = buildnode(counter);
		counter++;
		curr->left = leftnode;
		q1.push(leftnode);
		
		if(counter<n)
		{
			struct node* rightnode = buildnode(counter);
			counter++;
			curr->right = rightnode;
			q1.push(rightnode);
		}
	}
	
	return root;
	
	
	
}

void bfs(struct node*root)
{
	queue<struct node*> q1;
	q1.push(root);
	
	while(!q1.empty()){
		
		zsim_graph_node((uint64_t)q1.front());
		struct node* curr = q1.front();
		int val = *(int*)(void*)curr;
		//val = val & 0x01;
		//printf("Node %p isleaf %p %s\n", curr, (bool*)(void*)curr+4, val ? "true" : "false");
		printf("Node %p isleaf %p %d\n", curr, (int*)(void*)curr, val);
		q1.pop();
		cout<<curr->value<<" "<<curr->isleafnode<<endl;
		
		if(curr->left)
			q1.push(curr->left);
		if(curr->right)
			q1.push(curr->right);

	}
	
	return;
	
	
}


int main(int argc, char *argv[]) {
	// your code goes here
	int n = 10;
	if (argc > 1) {
		
	    n = atoi(argv[1]);
	}
	
	struct node *graph = buildgraph(n);
	zsim_roi_begin();
	bfs(graph);
	zsim_roi_end();
	
	
	return 0;
}

