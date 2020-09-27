#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <json-c/json.h>
#include "users.h"
#include "list.h"
#include "queue.h"

#define PUERTO "2525"
#define DOMINIO "pucv.localdomain"
#define CLIENTES_MAX 10
#define QUEUE_SIZE 20
#define BUFFER_MAX 1024

#define MAX_EMAIL_LENGTH 320
#define MAX_LOCAL_LENGTH 64
#define MAX_DOMAIN_LENGTH 255
#define MAX_BODY_LENGTH 10000

// sudo apt install libjson-c-dev
//gcc server.c users.c list.c queue.c -o server -ljson-c -lpthread

//Lista enlazada simple no circular
struct lista
{
	int dato;
	struct lista* siguiente;
};

//Tipo de dato estructurado EMAIL
typedef struct email
{
	char remitente[MAX_EMAIL_LENGTH];
	char destinatario[MAX_EMAIL_LENGTH];
	char cuerpo[MAX_BODY_LENGTH];	
}Email;

//Variables globales
int maximo_conexiones;			//Cantidad maxima de conexiones simultaneas
struct lista* conexiones;		//Lista enlazada de conexiones (cliente)
List* usuarios;				//Lista enlazada de usuarios
Queue* emails;				//Cola de emails que van siendo enviados
pthread_t thread;			//Ultimo hilo ejecutado

void inicializar();
void* smtp(void* argumentos_thread);
void strToUpper(char*);

int main (int argc, char *argv[])
{
	struct sockaddr_storage direccion_cliente;
	socklen_t direccion_size = sizeof(direccion_cliente);
	int nuevo_cliente;
	
	fd_set sockets;
	struct lista* auxiliar;
	int* argumentos_thread;
	
	//Inicializa la cola de correos
	emails = createQueue(QUEUE_SIZE); 

	//Carga los usuarios del sistema a traves de un archivo JSON
	usuarios = createList();
	cargar_usuarios(usuarios, "usuarios.json");

	//Se inicializa el socket del servidor, dejandolo en modo escucha para futuras conexiones de clientes
	inicializar();

	//Loop infinito que se encuentra a la escucha de nuevas conexiones (cliente)
	for ( ; ; )
	{
		FD_ZERO(&sockets);

		for (auxiliar = conexiones; auxiliar != NULL; auxiliar = auxiliar->siguiente)
		{
			FD_SET(auxiliar->dato, &sockets);
		}

		//Queda a la espera por siempre de nuevas conexiones
		select(maximo_conexiones+1, &sockets, NULL, NULL, NULL);

		//Recorre toda la lista enlazada de sockets en busca de una nueva conexion
		for (auxiliar = conexiones; auxiliar != NULL; auxiliar = auxiliar->siguiente)
		{
			if (FD_ISSET(auxiliar->dato, &sockets))
			{
				nuevo_cliente = accept(auxiliar->dato, (struct sockaddr*) &direccion_cliente, &direccion_size);
			
				if (nuevo_cliente == -1)
				{
					fprintf(stderr, "[-] Error al aceptar la conexion del cliente");
					continue;
				}

				// Se reserva memoria dinamica para almacenar el descriptor del socket, para posteriormente enviarlo al hilo
				argumentos_thread = (int*) malloc(sizeof(int));

				*argumentos_thread = nuevo_cliente;

				//Crea un nuevo hilo, con la funcion correspondiente para manejar la conexion con el cliente, con el fin de tener conexiones simultaneas
				pthread_create(&thread, NULL, smtp, argumentos_thread);
			}
		}
	}

	return 0;
}

//Inicializa el socket del servidor, para dejarlo a la escucha de nuevas conexiones
void inicializar()
{
	int conexion, opcion = 1;
	struct addrinfo servidor;
	struct addrinfo* informacion_servidor;
	struct addrinfo* auxiliar;

	//Inicializa toda la informacion correspondiente al socket del servidor
	memset(&servidor, 0, sizeof(servidor));
	servidor.ai_family = AF_UNSPEC;
	servidor.ai_socktype = SOCK_STREAM;
	servidor.ai_flags = AI_PASSIVE;
	
	//Inicializa la lista enlazada simple
	conexiones = NULL;
	maximo_conexiones = 0;
	
	if ((getaddrinfo(NULL, PUERTO, &servidor, &informacion_servidor)) != 0)
	{
		fprintf(stderr, "[-] Error al obtener la informacion del host.");
		exit(EXIT_FAILURE);
	}

	for (auxiliar = informacion_servidor; auxiliar != NULL; auxiliar = auxiliar->ai_next)
	{
		conexion = socket(auxiliar->ai_family, auxiliar->ai_socktype, auxiliar->ai_protocol);

		if (conexion == -1)
		{
			fprintf(stderr, "[-] Error al crear el socket.\n");
			continue;
		}

		setsockopt(conexion, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(int));

		if (bind(conexion, auxiliar->ai_addr, auxiliar->ai_addrlen) == -1) 
		{
			close(conexion);
			fprintf(stderr, "[-] Error al bindear el socket.\n");
			continue;
		}

		if (listen(conexion, CLIENTES_MAX) == -1)
		{
			fprintf(stderr, "[-] Error al poner socket a la escucha.\n");
			exit(EXIT_FAILURE);
		}

		//Actualiza el numero de maximas conexiones
		if (conexion > maximo_conexiones)
			maximo_conexiones = conexion;
		else
			maximo_conexiones = 1;
			

		//Asigna memoria a lista enlazada simple de conexiones y agrega una nueva conexion (socket)
		struct lista* nueva_conexion = malloc(sizeof(struct lista));
		nueva_conexion->dato = conexion;
		nueva_conexion->siguiente = conexiones;
		conexiones = nueva_conexion;
	}

	if (conexiones == NULL)
	{
		fprintf(stderr, "[-] Error al tratar de bindear los sockets.");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(informacion_servidor);
}

//Funcion que va manejando la conexion segun el protocolo SMTP con los clientes entrantes
void* smtp(void* argumentos_thread) 
{
	int bytes, conexion, finalizado;
	char buffer_entrada[BUFFER_MAX];
	char buffer_salida[BUFFER_MAX];
	fd_set sockset;
	struct timeval tiempo;
	char* token;
	const char espacio[2] = " ";
	char comando[BUFFER_MAX];
	
	//Atributos para el email
	Email* email;
	char remitente[MAX_EMAIL_LENGTH];
	char destinatario[MAX_EMAIL_LENGTH];
	char cuerpo[MAX_BODY_LENGTH];
	
	//Traspasa los argumentos del hilo (descriptor del socket) y libera esa memoria
	conexion = *(int*) argumentos_thread;
	free(argumentos_thread);
	printf("[+] Iniciando nuevo hilo para el socket #%d\n", conexion);
	

	//Se envia el banner del servidor al cliente
	sprintf(buffer_salida, "220 %s ESMTP Postfix (Debian/GNU)\r\n", DOMINIO);
	printf("%s", buffer_salida);
	send(conexion, buffer_salida, strlen(buffer_salida), 0);

	//Bucle infinito para manejar la comunicación entre el cliente y el servidor (uso del protocolo SMTP)
	for ( ; ; )
	{
		//Inicializa los sockets y los buffer en 0
		FD_ZERO(&sockset);
		FD_SET(conexion, &sockset);
		bzero(buffer_entrada, BUFFER_MAX);
		bzero(buffer_salida, BUFFER_MAX);
		
		//Inicializa los intervalos de tiempo
		tiempo.tv_sec = 120;
		tiempo.tv_usec = 0;

		//Espera un intervalo de tiempo antes de mandar algo por parte del servidor
		select(conexion+1, &sockset, NULL, NULL, &tiempo);

		//Si no hay un input en un intervalo de tiempo, finaliza la conexion con el cliente
		if (!FD_ISSET(conexion, &sockset)) 
		{
			printf("[!] #%d: Socket timed out", conexion);
			sprintf(buffer_salida, "421 4.4.2 %s Error: timeout exceeded\r\n", DOMINIO);
			send(conexion, buffer_salida, strlen(buffer_salida), 0);
			break;
		}

		//Recibe el input desde el cliente
		bytes = recv(conexion, buffer_entrada, BUFFER_MAX, 0);

		if (bytes == 0)
		{
			fprintf(stderr, "[-] Se ha cerrado la conexion\n");
			break;
		}

		if (bytes == -1) 
		{
			fprintf(stderr, "[-] Error en la conexion\n");
			break;
		}

		//Si no se ingreso ningun comando, envia un mensaje de error
		if (strlen(buffer_entrada) < 3)
		{
			sprintf(buffer_salida, "500 5.5.2 Error: bad syntax\r\n");
			printf("Servidor #%d: %s", conexion, buffer_salida);
			send(conexion, buffer_salida, strlen(buffer_salida), 0);
			continue;
		}

		printf("Cliente #%d: %s\n", conexion, buffer_entrada);
		strncpy(comando, buffer_entrada, BUFFER_MAX);
		comando[strlen(comando)-2] = '\0';
		token = strtok(comando, espacio);
		strToUpper(token);

		if (strcmp(token, "HELO") == 0)
			sprintf(buffer_salida, "250 %s\r\n", DOMINIO);
		else if (strcmp(token, "EHLO") == 0)
			sprintf(buffer_salida,"250-%s\n250-PIPELINING\n250-SIZE 10240000\n250-VRFY\n250-ETRN\n250-STARTTLS\n250-ENHANCEDSTATUSCODES\n250-8BITMIME\n250-DSN\n250 SMTPUTF8\r\n", DOMINIO);
		else if (strcmp(token, "MAIL") == 0)
		{
			token = strtok(NULL, espacio);
			strToUpper(token);
			
			if (strcmp(token, "FROM:") == 0)
			{
				token = strtok(NULL, espacio);
				
 				if ((token != NULL) && (strlen(token) > 0))
				{
					//Se agrega el remitente
					strncpy(remitente, token, MAX_EMAIL_LENGTH);
					sprintf(buffer_salida, "250 2.1.0 Ok\r\n");
				}
				else
					sprintf(buffer_salida, "501 5.5.4 Syntax: MAIL FROM:<address>\r\n");
			}
			else
				sprintf(buffer_salida, "501 5.5.4 Syntax: MAIL FROM:<address>\r\n");
		}
		else if (strcmp(token, "RCPT") == 0)
		{
			//Es necesario especificar el remitente primero, con MAIL FROM
			if ((remitente == NULL) || (strlen(remitente) == 0))
				sprintf(buffer_salida, "503 5.5.1 Error: need MAIL command\r\n");
			else
			{
				token = strtok(NULL, espacio);
				strToUpper(token);
				
				if (strcmp(token, "TO:") == 0)
				{
					token = strtok(NULL, espacio);
					
					if ((token != NULL) && (strlen(token) > 0))
					{
						if (buscar_usuario(usuarios, token))
						{
							//Se agrega el destinatario
							strncpy(destinatario, token, MAX_EMAIL_LENGTH);
							sprintf(buffer_salida, "250 2.1.5 Ok\r\n");
						}
						else
						{
							//Si el usuario no es valido, es decir, no existe en el sistema
							sprintf(buffer_salida, "550 5.1.1 <%s>: Recipient address rejected: User unknown in local recipient table\r\n", token);
						}
					}
					else
						sprintf(buffer_salida, "501 5.5.4 Syntax: RCPT TO:<address>\r\n");
				}
				else
					sprintf(buffer_salida, "501 5.5.4 Syntax: RCPT TO:<address>\r\n");
			}
		}
		else if (strcmp(token, "DATA") == 0)
		{
			//Es necesario especificar el destinatario primero, con RCPT TO
			if ((destinatario == NULL) || (strlen(destinatario) == 0))
				sprintf(buffer_salida, "503 5.5.1 Error: need RCPT command\r\n");
			else
			{
				sprintf(buffer_salida, "354 End data with <CR><LF>.<CR><LF>\r\n");
				printf("Servidor #%d: %s", conexion, buffer_salida);
				send(conexion, buffer_salida, strlen(buffer_salida), 0);
				finalizado = 0;
				
				//Se asigna el cuerpo del email, bucle finaliza solo si el usuario tipea un punto, luego un enter
				for ( ; ; )
				{
					bzero(buffer_entrada, BUFFER_MAX);
					bytes = recv(conexion, buffer_entrada, BUFFER_MAX, 0);
					
					//Verifica si el usuario a tipeado el final de cuerpo
					if ((finalizado) && (strlen(buffer_entrada) < 3))
						break;
					
					if (strcmp(buffer_entrada, ".\r\n") == 0)
						finalizado = 1;
					else
						finalizado = 0;
					
					
					//Si se excede del maximo de cuerpo permitido, se sale automaticamente
					if ((strlen(buffer_entrada) + strlen(cuerpo)) >= 10000)
						break;
					else
						strcat(cuerpo, buffer_entrada);
				}
				
				//Se crea un nuevo dato de tipo Email
				email = (Email*) malloc(sizeof(Email));
				strncpy(email->remitente, remitente, MAX_EMAIL_LENGTH);
				strncpy(email->destinatario, destinatario, MAX_EMAIL_LENGTH);
				strncpy(email->cuerpo, cuerpo, MAX_BODY_LENGTH);
				
				//Se agrega ese dato a la cola de emails
				push_queue(emails, email);
				
				sprintf(buffer_salida, "250 2.0.0 Ok: queued as BD8B4406F9\r\n");
			}
		}
		else if (strcmp(token, "RSET") == 0)
		{
			//Resetea los parametros
			remitente[0] = '\0';
			destinatario[0] = '0';
			sprintf(buffer_salida, "250 2.0.0 Ok\r\n");
		}
		else if (strcmp(token, "VRFY") == 0)
		{
			token = strtok(NULL, espacio);
			
			//Busca si existe el usuario en el sistema
			if (buscar_usuario(usuarios, token))
				sprintf(buffer_salida, "252 2.0.0 %s\r\n", token);
			else
				sprintf(buffer_salida, "550 5.1.1 <%s>: Recipient address rejected: User unknown in local recipient table\r\n", token);
		}
		else if (strcmp(token, "NOOP") == 0)
		{
			//No realiza nada, operacion NOP
			sprintf(buffer_salida, "250 2.0.0 Ok\r\n");
		}
		else if (strcmp(token, "QUIT") == 0)
		{
			sprintf(buffer_salida, "221 2.0.0 Bye\r\n");
			printf("S%d: %s", conexion, buffer_salida);
			send(conexion, buffer_salida, strlen(buffer_salida), 0);
			break;
		}
		else
			sprintf(buffer_salida, "502 5.5.2 Error: command not recognized\r\n");
		
		printf("Servidor #%d: %s", conexion, buffer_salida);
		send(conexion, buffer_salida, strlen(buffer_salida), 0);
	}

	//Se cierra la conexion y se libera el hilo
	close(conexion);
	pthread_exit(NULL);
}

void strToUpper(char* cadena)
{
    for (unsigned short int i=0; i<strlen(cadena); i++)
    {
        if (islower(cadena[i]))
            cadena[i] = toupper(cadena[i]);
    }
}
