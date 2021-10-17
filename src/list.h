#include <stdlib.h>

#ifndef LIST
#define LIST

typedef struct Node node;
typedef struct list List;

node* createNode(void* data);
List* createList();
void* firstList(List* lista);
void* nextList(List* lista);
void* prevList(List* lista);
void* lastList(List* lista);
void pushFront(List* lista, void* dato);
void pushBack(List* lista, void* dato);
void pushCurrent(List* lista, void* dato);
void popFront(List* lista);
void popBack(List* lista);
void popCurrent(List* lista);
void cleanList(List* lista);

#endif
