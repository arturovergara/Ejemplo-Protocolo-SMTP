#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

typedef struct queue
{
    void** data;
    int head;
    int tail; 
    int size;
}Queue;

Queue* createQueue(int size)
{
    Queue* cola=(Queue*) calloc(size,sizeof(Queue));
    cola->data=(void**) calloc(size,sizeof(void*));

    cola->size=size;
    cola->head=-1;
    cola->tail=-1;

    return cola;
}

//Se añade un elemento a la cola. Se añade al final de esta.
void push_queue(Queue* Q, void* data)   
{

    if (Q->tail==Q->size)
    {
        Q->data=(void*) realloc(Q->data, sizeof(void*)*(Q->size+1));      //AUMENTA UN ESPACIO CUANDO está llena
        Q->size++;
    }

    Q->tail=Q->tail+1;          // AUMENTA EN UNO LOS ELEMENTOS DE LA COLA
    Q->data[Q->tail]=data;      // INGRESA EL ELEMENTO EN LA COLA
}

// se elimina el elemento frontal de la cola, es decir, el primer elemento que entró.
void* pop_queue(Queue* Q)        
{
    if (Q->head!=(Q->tail))
	{
        Q->head = (Q->head+1)%Q->size;
    }
}