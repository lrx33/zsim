#ifndef _GRAPHETCH_GRAPHNODE_
#define _GRAPHETCH_GRAPHNODE_

struct node {
    int id;
    int value1;
    int value2;
    int value3;
    int value4;

    struct node *left, *right;
};

#endif
