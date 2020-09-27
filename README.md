# Ejemplo-Protocolo-SMTP-en-C
Pequeño servidor SMTP no funcional, creado de manera didáctica para estudiar conceptos de networking

### Comandos SMTP Implementados
Se han implementado solamente los siguientes comandos SMTP:

| Comando | Descripción |
| ----- | ---- |
| HELO | El cliente se indentifica ante el servidor. |
| EHLO | Otra manera de indentificarse ante el servidor. | 
| MAIL FROM: <user> | Especifica el remitente del correo. |
| RCPT TO: <user> | Especifica el destinatario del correo. |
| DATA | Especifica todo el cuerpo del correo hasta que el usuario tipee ". \n" |
| VRFY <user> | Verifica si el usuario especificado existe en el sistema. |
| NOOP | No operation, no realiza nada. |
| QUIT | Finaliza la comunicación con el servidor. | 

### Dependencias
`sudo apt install libjson-c-dev`

### Compilar
`gcc server.c users.c list.c queue.c -o server -ljson-c -lpthread`

