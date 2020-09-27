#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "users.h"
#include "list.h"

#define MAX 1024
#define USER_LENGTH 32
#define MAIL_LENGTH 320

typedef struct usuario
{
	unsigned short int id;
	char username[USER_LENGTH];
	char mail[MAIL_LENGTH];
}Usuario;


//Busca en la lista el usuario por username o mail
unsigned char buscar_usuario(List* lista, const char* termino)
{
	Usuario* auxiliar;
	Usuario* primero;
	unsigned char encontrado = 0;
	
	auxiliar = firstList(lista);
	primero = auxiliar;
	
	//Se recorre la lista enlazada
	while (auxiliar != NULL)
	{
		if ((strcmp(auxiliar->username, termino) == 0) || (strcmp(auxiliar->mail, termino) == 0))
		{
			encontrado = 1;
			break;
		}
		
		auxiliar = nextList(lista);
		
		if (auxiliar == primero)
			break;
	}
	
	return encontrado;
}


//Carga los usuarios del sistema (SMTP)
void cargar_usuarios(List* lista, const char* file_name)
{
	FILE* archivo;
	char buffer[MAX];
	Usuario* dato;
	unsigned short int cantidad_usuarios;
	
	// Estructuras JSON
	struct json_object* usuarios;
	struct json_object* usuario;
	struct json_object* id;
	struct json_object* username;
	struct json_object* mail;
	
	archivo = fopen(file_name, "r");
	
	if (archivo == NULL)
	{
		fprintf(stderr, "[-] Error al cargar los usuarios, al intentar abrir el archivo.");
		exit(EXIT_FAILURE);
	}
	
	fread(buffer, MAX, 1, archivo);
	fclose(archivo);
	
	usuarios = json_tokener_parse(buffer);
	cantidad_usuarios = json_object_array_length(usuarios);
	
	for (unsigned short int i=0; i < cantidad_usuarios; i++)
	{
		usuario = json_object_array_get_idx(usuarios, i);
		json_object_object_get_ex(usuario, "id", &id);
		json_object_object_get_ex(usuario, "username", &username);
		json_object_object_get_ex(usuario, "mail", &mail);
		
		dato = (Usuario*) malloc(sizeof(Usuario));
		
		dato->id = json_object_get_int(id);
		strcpy(dato->username, json_object_get_string(username));
		strcpy(dato->mail, json_object_get_string(mail));
		pushBack(lista, dato);
	}
}

