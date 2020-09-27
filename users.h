#include "list.h"

#ifndef USER
#define USER
#define USER_LENGTH 32
#define MAIL_LENGTH 320

typedef struct usuario Usuario;

unsigned char buscar_usuario(List*, const char*);
void cargar_usuarios(List*, const char*);
#endif
