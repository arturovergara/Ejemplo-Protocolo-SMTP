# Ejemplo-Protocolo-SMTP-en-C
Pequeño servidor SMTP no funcional, creado de manera didáctica para estudiar conceptos de networking

# Dependencias
sudo apt install libjson-c-dev

# Para compilar se debe utilizar el sigueinte comando
gcc server.c users.c list.c queue.c -o server -ljson-c -lpthread

