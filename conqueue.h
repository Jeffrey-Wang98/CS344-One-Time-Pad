#ifndef CONQUEUE_H_
#define CONQUEUE_H_


struct node {
  struct node* next;
  int* socketPtr;
};
typedef struct node node_t;

void enqueue(int* socketPtr);

int* dequeue();

#endif
