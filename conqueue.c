#include "conqueue.h"
#include <stdlib.h>

node_t* head = NULL;
node_t* tail = NULL;


void
enqueue(int* socketPtr) {
  node_t* newNode = malloc(sizeof(node_t));
  // Assign new data
  newNode->socketPtr = socketPtr;
  newNode->next = NULL;
  // Put new node to end of linked list
  if (tail == NULL) {
    head = newNode;
  }
  else {
    tail->next = newNode;
  }
  tail = newNode;
}

int*
dequeue() {
  if (head == NULL) return NULL;
  node_t* tempHead = head;
  int* outSocket = head->socketPtr;
  head = head->next;
  // if new head is NULL, tail is NULL
  if (head == NULL) tail = NULL;
  free(tempHead);
  return outSocket;
}

