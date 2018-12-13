#ifndef _GRAPHETCH_GRAPHNODE_
#define _GRAPHETCH_GRAPHNODE_

struct node {
    int32_t id;
    int32_t values[5];

    struct node *left, *right;
};

#endif
