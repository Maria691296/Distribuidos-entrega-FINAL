#ifndef SERVER_C
#define SERVER_C

#define MAX_TOKENS 10
#define MAX_HILOS 5

#include <stdlib.h>

typedef struct param {
    int newsd;
    int port;
    char ip[16];
} petition_t;

// Envío de mensajes mediante sockets
#include "db.h"
#include "server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>


sem_t semaforo;


// ========================================================================
// CREACIÓN DE UN SOCKET
// ========================================================================


// Obtiene el descriptor de socket y lo retorna
int create_socket();

// Toma el puerto como entero y la ip en formato X.X.X.X y retorna la configuración hecha
struct sockaddr_in configure_socket(char *local_ip, int local_port);



// ========================================================================
// FUNCIONES DE COMUNICACIÓN
// ========================================================================


// Manda los bytes de total, ni uno más, no añade '\0' al final de sus cadenas
int sendMessage (int newsd, char *buffer, size_t total);

// No controla el tamaño de la entrada, termina cuando encuentra '\0'
int readMessage (int newsd, char *buffer);

// Lee exactamente 1 byte del socket
uint8_t readCode(int socket);

// Manda un número como cadena de caracteres
int sendNumber(int socket, int number);


// ========================================================================
// PETICIONES
// ========================================================================


// 
void my_connect(int newsd, char *peer_ip);

// 
void my_disconnect(int newsd, char *peer_ip);

// 
void my_register(int newsd);

// 
void my_unregister(int newsd);

// 
void my_users(int newsd);

// Función que se ejecuta cuando el usuario manda un SEND
// Nota: Resulta pesada para la base de datos, se tiene que adueñar un rato del mutex para que nadie modifique nada mientras trabaja
void my_send(int newsd, int is_sendattach);

// Mandarle la confirmación al usuario de que se ha enviado su mensaje
// 
int my_send_ACK (message_t *message);

// Mandar mensajes cuando el destinatario está conectado
// 0 = OK // 1 = SOCKET_FAIL
int my_send_connected (char *user, message_t *message);



// ========================================================================
// FUNCION DEL HILO
// ========================================================================


// Código del thread que atiende las peticiones
void *atender_peticion ( void *arg );



// ========================================================================
// FUNCION MAIN
// ========================================================================


int main ( int argc, char **argv );



#endif