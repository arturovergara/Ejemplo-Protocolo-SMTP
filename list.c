#include <stdlib.h>

typedef struct Node
{
	void* dato;
	struct Node* prev;
	struct Node* next;
}node;

typedef struct list
{
	node* first;
	node* current;
	int size;
}List;

node* createNode(void* dato)
{
	node* nodo = (node*) malloc(sizeof(node));

	if (nodo != NULL)
	{
		nodo->dato = dato;
		nodo->prev = NULL;
		nodo->next = NULL;
	}

	return nodo;
}

List* createList()
{
	List* lista = (List*) calloc(1, sizeof(List));
	
	lista->first = NULL;
	lista->current = NULL;
	lista->size = 0;

	return lista;
}

void* firstList(List* lista)
{
	if ((lista == NULL) || (lista->first == NULL))
		return NULL;

	lista->current = lista->first;

	return lista->current->dato;
}

void* nextList(List* lista)
{
	if ((lista == NULL) || (lista->first == NULL))
		return NULL;

	lista->current = lista->current->next;

	return lista->current->dato;
}

void* prevList(List* lista)
{
	if ((lista == NULL) || (lista->current->prev == NULL))
		return NULL;

	lista->current = lista->current->prev;

	return lista->current->dato;
}

void* lastList(List* lista)
{
	if ((lista == NULL) || (lista->first == NULL))
		return NULL;

	lista->current = lista->first->prev;

	return lista->current->dato;
}

void pushFront(List* lista, void* dato)
{
	node* nodo = createNode(dato);

	if (lista->first != NULL)
	{
		nodo->next = lista->first;
		nodo->prev = lista->first->prev;
		lista->first->prev->next = nodo;
		lista->first->prev = nodo;
		lista->first = nodo;
	}
	else
	{
		lista->first = nodo;
		lista->current = nodo;
		nodo->next = nodo;
		nodo->prev = nodo;
	}

	lista->size++;
}

void pushBack(List* lista, void* dato)
{
	node* nodo = createNode(dato);

	if (lista->first != NULL)
	{
		nodo->prev = lista->first->prev;
		nodo->next = lista->first;
		lista->first->prev->next = nodo;
		lista->first->prev = nodo;
	}
	else
	{
		lista->first = nodo;
		lista->current = nodo;
		nodo->next = nodo;
		nodo->prev = nodo;
	}

	lista->size++;
}

void pushCurrent(List* lista, void* dato)
{
	node* nodo = createNode(dato);

	if (lista->first != NULL)
	{
		nodo->prev = lista->current;
		nodo->next = lista->current->next;
		lista->current->next->prev = nodo;
		lista->current->next = nodo;
		lista->current = nodo;
	}
	else
	{
		lista->first = nodo;
		lista->current = nodo;
		nodo->next = nodo;
		nodo->prev = nodo;
	}

	lista->size++;
}

void popFront(List* lista)
{
	node* auxiliar = lista->first;

	if (lista->first->next == lista->first)
	{
		lista->first = NULL;
		lista->current = NULL;
		lista = NULL;
		free(auxiliar);
		return;
	}

	lista->first->next->prev = lista->first->prev;
	lista->first->prev->next = lista->first->next;
	lista->first = lista->first->next;
	free(auxiliar);
}

void popBack(List* lista)
{
	node* auxiliar = lista->first->prev->prev;

	if (lista->first->prev == NULL)
	{
		lista->first = NULL;
		lista->current = NULL;
	}
	else
	{
		lista->first->prev->prev->next = lista->first;
		lista->first->prev = lista->first->prev->prev;
	}

	free(auxiliar->next);
	lista->first->prev = auxiliar;
	lista->size--;
}

void popCurrent(List* lista)
{
	if (lista->first != NULL)
	{
		node* auxiliar = lista->current;

		if ((lista->current == lista->first) && (lista->size))
		{
			popFront(lista);
			lista->size--;
			return;
		}

		if (auxiliar != auxiliar->next)
		{
			auxiliar->prev->next = auxiliar->next;
			auxiliar->next->prev = auxiliar->prev;
			lista->current = auxiliar->next;

			if (lista->first == lista->current)
				lista->first = auxiliar->next;
		}
		else
		{
			lista->first = NULL;
			lista->current = NULL;
			lista->size = 0;
		}

		lista->size--;
		free(auxiliar->dato);
		free(auxiliar);
		return;
	}
}

void cleanList(List* lista)
{
	while (lista->first != NULL)
	{
		lista->first = lista->first->next;
		popFront(lista);
	}
}
