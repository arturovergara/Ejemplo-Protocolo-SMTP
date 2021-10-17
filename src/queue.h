#ifndef QUEUE_H
#define QUEUE_H


typedef struct queue Queue;

Queue* createQueue(int size);                   // RETORNA COLA VACIA

void push_queue(Queue* Q, void* data);             // INGRESA UN ELEMENTO AL PRINCIPIO DE LA COLA

void* pop_queue(Queue* Q);                         // ELIMINA EL ULTIMO ELEMENTO DE LA COLA


#endif // QUEUE_H
