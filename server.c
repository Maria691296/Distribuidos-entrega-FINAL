#include "db.h"
#include "server.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>


sem_t semaforo;
char *LOG_RPC_IP;


// ========================================================================
// COMUNICACIÓN CON RPC
// ========================================================================

//
void rpc_log(char *user, char *operation, char *filename) {
    if (LOG_RPC_IP == NULL) {
        printf("s> LOG_RPC_IP no configurada, no se loguea\n");
        return;
    }

    CLIENT *clnt;
    int result;
    enum clnt_stat retval;

    // Crear conexión RPC al log_server
    clnt = clnt_create(LOG_RPC_IP, LOG, LOGVER, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(LOG_RPC_IP);
        return;
    }

    // Llamar a log_operation
    retval = log_operation_1(user, operation, filename, &result, clnt);
    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "call failed");
    }

    clnt_destroy(clnt);
}


// ========================================================================
// CREACIÓN DE UN SOCKET
// ========================================================================


// Obtiene el descriptor de socket y lo retorna
int create_socket() {

    // Aquí almacenaremos el socket
    int sd;

    // Creamos el socket descriptor en IPV4 con TCP
    sd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sd < 0) {
        perror("Error en la creación del socket: ");
        return -1;
    }

    return sd;
}

// Toma el puerto como entero y la ip en formato X.X.X.X y retorna la configuración hecha
struct sockaddr_in configure_socket(char *local_ip, int local_port) {
    
    // Esta es la "hoja de configuración" que necesita el socket
    struct sockaddr_in local_addr;

    // Limpiamos los datos basura de la RAM
    bzero((char *)&local_addr, sizeof(local_addr));

    // Configuramos el socket
    local_addr.sin_family = AF_INET; // Nuestro socket es IPv4
    local_addr.sin_port   = htons(local_port); // Dónde nos alojaremos
    inet_pton(AF_INET, local_ip, &local_addr.sin_addr); // Copiamos la IP en local_addr

    return local_addr;
}



// ========================================================================
// FUNCIONES DE COMUNICACIÓN
// ========================================================================


// Manda "total" bytes, sin excepción, garantizar limpieza del buffer
int sendMessage (int newsd, char *buffer, size_t total) {
    size_t escritos = 0 ;
    ssize_t result  = 0 ;

   // no vale "return write(newsd, buffer, total) ;" porque
   // puede que write NO escriba todo lo solicitado de una vez

    while (escritos != total) // mientras queda por escribir...
    {
        result = write(newsd, buffer+escritos, total-escritos) ;
        if (-1 == result) {
            return -1 ;
        }

        escritos = escritos + result ;
    }

    return escritos ;
}


// Lee hasta encontrar '\0', no es necesario limpiarlo
int readMessage (int newsd, char *buffer) {
    char byte = 0 ;
    int ntoken = 0;

    while (read(newsd, &byte, 1)) // mientras queda por leer...
    {
        buffer[ntoken] = byte;
        ntoken++;
        
        if (byte == '\0') {
            return 0;
        }
    }

    return -1 ;
}


// Lee exactamente 1 byte del socket
uint8_t readCode(int socket) {
    uint8_t code = 0;
    ssize_t n = read(socket, &code, 1);
    
    // Si la lectura da error...
    if (n <= 0) return 3;
    
    return code;
}


// Envía la cadena del número acabada en '\0'
int sendNumber(int socket, int number) {
    char buffer[24];
    sprintf(buffer, "%d", number);
    sendMessage(socket, buffer, strlen(buffer)+1);
}


// ========================================================================
// PETICIONES
// ========================================================================


// 
void my_connect(int newsd, char *peer_ip) {
    // DEBUG
    // printf("CONNECT OP IN COURSE\n");

    // Declaración de variables necesarias
    int codigo_respuesta = -1;
    int codigo_connected = -1;
    int listen_port = -1;
    char user[MAX_STR_LEN];
    char port[MAX_STR_LEN];
    message_t *message_ptr = NULL;

    // Recibimos la cadena que representa al usuario
    readMessage(newsd, user);
    readMessage(newsd, port);
    listen_port = atoi(port);

    // Registramos los datos de operación con RPC
    rpc_log(user, "CONNECT", "");

    // Ejecutamos la función real sobre la db
    codigo_respuesta = connect_user(user, peer_ip, listen_port);

    // Mandamos el código de respuesta de vuelta
    sendMessage(newsd, (char *) &codigo_respuesta, 1);

    if (codigo_respuesta == 0) {
        printf("s> CONNECT %s OK\n", user);

        // Tocamos la db, activamos el mutex
        pthread_mutex_lock(&lock_db);
        
        // Sacamos el primer mensaje
        message_ptr = pop_first_message(user);

        // Por cada mensaje, hasta que se acaben o hasta que uno de fallo
        while (message_ptr != NULL) {
            codigo_connected = my_send_connected(user, message_ptr);
            if (codigo_connected != 0) { insert_first_message(user, message_ptr); break; }
            my_send_ACK(message_ptr);
            message_ptr = pop_first_message(user);
        }

        pthread_mutex_unlock(&lock_db);

    }
    else {printf("s> CONNECT %s FAIL\n", user);}
}


// 
void my_disconnect(int newsd) {
    // Declaración de variables necesarias
    int codigo_respuesta = -1;
    char user[MAX_STR_LEN];

    // Recibimos la cadena que representa al usuario
    readMessage(newsd, user);

    // Registramos los datos de operación con RPC
    rpc_log(user, "DISCONNECT", "");

    // Ejecutamos la funcionalidad real sobre la db
    codigo_respuesta = disconnect_user(user);

    if (codigo_respuesta == 0) printf("s> DISCONNECT %s OK\n", user);
    else printf("s> DISCONNECT %s FAIL\n", user);

    // Mandamos el código de respuesta de vuelta
    sendMessage(newsd, (char *) &codigo_respuesta, 1);
}


// 
void my_register(int newsd) {
    // Declaración de variables necesarias
    int codigo_respuesta = -1;
    char user[MAX_STR_LEN];

    // Recibimos la cadena que representa al usuario
    readMessage(newsd, user);

    // Ejecutamos el código real de la db
    codigo_respuesta = insert_user(user);

    // Registramos los datos de operación con RPC
    rpc_log(user, "REGISTER", "");

    // Feedback por terminal del servidor
    if (codigo_respuesta == 0) printf("s> REGISTER %s OK\n", user);
    else printf("s> REGISTER %s FAIL\n", user);


    // Mandamos el código de respuesta de vuelta
    sendMessage(newsd, (char *) &codigo_respuesta, 1);
}


// 
void my_unregister(int newsd) {
    // Declaración de variables necesarias
    int codigo_respuesta = -1;
    char user[MAX_STR_LEN];

    // Recibimos la cadena que representa al usuario
    readMessage(newsd, user);

    // Registramos los datos de operación con RPC
    rpc_log(user, "UNREGISTER", "");

    // Ejecutamos el código real de la db
    codigo_respuesta = delete_user(user);

    if (codigo_respuesta == 0) printf("s> UNREGISTER %s OK\n", user);
    else printf("s> UNREGISTER %s FAIL\n", user);

    // Mandamos el código de respuesta de vuelta
    sendMessage(newsd, (char *) &codigo_respuesta, 1);
}


// 
void my_users(int newsd) {
    
    // Declaración de variables necesarias
    int codigo_respuesta = -1;
    char buffer[MAX_STR_LEN*3];
    char user[MAX_STR_LEN];
    user_t **user_list;

    // Recibimos la cadena que representa al usuario
    readMessage(newsd, user);
    

    // Acceso protegido a la db
    pthread_mutex_lock(&lock_db);

    // Registramos los datos de operación con RPC
    rpc_log(user, "USERS", "");

    codigo_respuesta = check_user_status(user);
    if (codigo_respuesta == 0) user_list = return_connected();

    pthread_mutex_unlock(&lock_db);

    // Mandamos el código de respuesta de vuelta
    sendMessage(newsd, (char *) &codigo_respuesta, 1);

    // Si no está conectado, salimos
    if (codigo_respuesta != 0) { printf("s> CONNECTEDUSERS FAIL\n"); return; }

    // Si está conectado mandamos los usuarios uno por uno
    printf("s> CONNECTEDUSERS OK\n");

    // Avisamos al cliente de los mensajes que va a recibir
    sendNumber(newsd, my_db.connected_users);

    // Y le mandamos cada usuario por separado
    for (int i = 0; i < my_db.connected_users; i++) {
        sprintf(buffer, "%s::%s::%d", user_list[i]->user, user_list[i]->return_ip, user_list[i]->return_port);
        sendMessage(newsd, buffer, strlen(buffer)+1);
        free(user_list[i]);
    }

    // Liberamos el espacio reservado al array de punteros
    free(user_list);
}


// Función que se ejecuta cuando el usuario manda un SEND o SENDATTACH
// Nota: Resulta pesada para la base de datos, se tiene que adueñar un rato del mutex para que nadie modifique nada mientras trabaja
void my_send(int newsd, int is_sendattach) {

    // Declaración de variables necesarias
    int codigo_respuesta = -1;
    int state_user_from = -1;
    int state_user_to = -1;
    char buffer[MAX_STR_LEN];
    char user_from[MAX_STR_LEN];
    char user_to[MAX_STR_LEN];
    char message[MAX_STR_LEN];
    char filename[MAX_STR_LEN] = ""; // Iniciado a nulo para que SEND funcione
    message_t *message_ptr = NULL;

    // Recibimos a los usuarios implicados, el mensaje
    readMessage(newsd, user_from);
    readMessage(newsd, user_to);
    readMessage(newsd, message);
    if (is_sendattach == 1) {
        readMessage(newsd, filename);
        // Registramos los datos de operación con RPC
        rpc_log(user_from, "SENDATTACH", filename);
    }

    else {
        // Registramos los datos de operación con RPC
        rpc_log(user_from, "SEND", "");
    }
    

    // DEBUG
    // printf("%s -> %s\n", user_from, user_to);
    // printf("SEND/SENDATTACH code is: %d\n", is_sendattach);

    // COMPROBACIONES INTERNAS

    // Adquirimos el mutex para evitar problemas de condiciones de carrera en la db
    pthread_mutex_lock(&lock_db);

    // Comprobamos que los usuarios estén en la db
    state_user_from = check_user_status(user_from);
    state_user_to = check_user_status(user_to);

    // Si alguno de los dos no existe, lanzamos error 1
    if (state_user_from == 2 || state_user_to == 2) {
        pthread_mutex_unlock(&lock_db);
        codigo_respuesta = 1; 
        sendMessage(newsd, (char *) &codigo_respuesta, 1);
        return;
    }

    // Actualizar el contador y verificar valor máximo
    user_t *user_from_ptr = search_user(user_from);
    unsigned int id_mensaje = ++user_from_ptr->message_id;
    if (id_mensaje == UINT_MAX) user_from_ptr->message_id = 0;

    // Almacenamos el mensaje en la db
    insert_message(user_to, id_mensaje, user_from, message, filename);

    // Mandamos el código de respuesta de vuelta (el servidor ha recibido el mensaje)
    codigo_respuesta = 0;
    sendMessage(newsd, (char *) &codigo_respuesta, 1);
    sendNumber(newsd, id_mensaje);

    // Si el usuario receptor está conectado
    if (state_user_to == 0) {
        // Le mandamos el mensaje inmediatamente
        message_ptr = pop_last_message(user_to);
        codigo_respuesta = my_send_connected(user_to, message_ptr);

        // Si se logra enviar el mensaje, notificamos al remitente y liberamos el mensaje
        if (codigo_respuesta == 0) { my_send_ACK(message_ptr); free(message_ptr); }

        // Si no ha habido éxito, reinsertamos el mensaje donde estaba
        if (codigo_respuesta != 0) {
            insert_last_message(user_to, message_ptr);
        }
    }
    pthread_mutex_unlock(&lock_db);
}


// Mandar mensajes de tipo SEND o SENDATTACH cuando el destinatario está conectado
// 0 = OK // 1 = SOCKET_FAIL
int my_send_connected (char *user, message_t *message) {

    // Creamos el socket descriptor del s-s-c
    int newsd = create_socket();
    if (newsd < 0) return 1;

    // Obtenemos los datos de la conexión del usuario para saber dónde mandar los datos
    user_t *user_ptr = search_user(user);
    struct sockaddr_in remote_addr = configure_socket(user_ptr->return_ip, user_ptr->return_port);

    // Nos conectamos al thread
    int ret = connect(newsd, (struct sockaddr *) &remote_addr, sizeof(remote_addr)) ;
    if (ret < 0) {
        perror("ERROR en connect: ");
        return 1;
    }

    // Si el mensaje está asociado a un SEND...
    if (strcmp(message->filename, "") == 0) {
        char *op = "SEND_MESSAGE";
        sendMessage(newsd, op, strlen(op) + 1);
        sendMessage(newsd, message->transmitter, strlen(message->transmitter) + 1);
        sendNumber(newsd, message->id);
        sendMessage(newsd, message->message, strlen(message->message) + 1);
    }
    // Si el mensaje está asociado a un SENDATTACH...
    else {
        char *op = "SEND_MESSAGE_ATTACH";
        sendMessage(newsd, op, strlen(op) + 1);
        sendMessage(newsd, message->transmitter, strlen(message->transmitter) + 1);
        sendNumber(newsd, message->id);
        sendMessage(newsd, message->message, strlen(message->message) + 1);
        sendMessage(newsd, message->filename, strlen(message->filename) + 1);
    }

    // Damos feedback del mensaje enviado
    printf("\ns> SEND MESSAGE %d FROM %s TO %s\n", message->id, message->transmitter, user);  // TODO COMPROBAR PRINT GENÉRICO ESTA BIEN??

    // Cerramos la conexión al thread de escucha
    close(newsd);

    return 0;
}


// Mandarle la confirmación de tipo SEND o SENDATTACH al usuario de que se ha enviado su mensaje
// 
int my_send_ACK (message_t *message) {
    
    // Creamos el socket descriptor del s-s-c
    int ssc_sd = create_socket();
    if (ssc_sd < 0) return 1;

    char buffer[MAX_STR_LEN];

    // Obtenemos los datos del css
    user_t *user_ptr = search_user(message->transmitter);

    // TODO Comprobación redundante
    if (user_ptr->connected == 0) return 1;

    // El destino del socket es lo que nos haya mandado el cliente durante el connect
    struct sockaddr_in remote_addr = configure_socket(user_ptr->return_ip, user_ptr->return_port);

    // Nos conectamos usando el nuevo socket
    int ret = connect(ssc_sd, (struct sockaddr *) &remote_addr, sizeof(remote_addr)) ;
    if (ret < 0) {
        perror("ERROR en connect: ");
        return 1;
    }

    // Si el mensaje está asociado a un SEND...
    if (strcmp(message->filename, "") == 0) {
        // Mandamos la operación y el identificador del mensaje
        strcpy(buffer, "SEND_MESS_ACK");
        sendMessage(ssc_sd, buffer, strlen(buffer) + 1);
        sendNumber(ssc_sd, message->id);
    }
    // Si el mensaje está asociado a un SENDATTACH
    else { // SENDATTACH
        strcpy(buffer, "SEND_MESS_ATTACH_ACK");
        sendMessage(ssc_sd, buffer, strlen(buffer) + 1);
        sendNumber(ssc_sd, message->id);
        sendMessage(ssc_sd, message->filename, strlen(message->filename) + 1);
    }

    // Cerramos la conexión
    close(ssc_sd);

    return 0;
}



// ========================================================================
// FUNCION DEL HILO
// ========================================================================


// Código del thread que atiende las peticiones
void *atender_peticion ( void *arg )
{

    // Extraemos los datos del peer (cliente) liberamos su memoria dinámica
    petition_t *args = (petition_t*) arg;

    int newsd = args->newsd;
    int peer_port = args->port;
    char peer_ip[16];
    strncpy(peer_ip, args->ip, sizeof(peer_ip));
    
    free(arg);

    // DEBUG
    // printf("Hilo con sd %d apuntando a %s::%d\n", newsd, peer_ip, peer_port);

    // Leemos la operación que envía el cliente
    char buffer[MAX_STR_LEN];
    readMessage(newsd, buffer);

    // DEBUG
    // printf("Operacion %s recibida con strcmp = %d\n", buffer, strcmp("SEND", buffer));

    // Y seleccionamos el manejador adecuado

    if (strcmp("CONNECT\0", buffer) == 0) my_connect(newsd, peer_ip);

    if (strcmp("DISCONNECT", buffer) == 0) my_disconnect(newsd);

    if (strcmp("REGISTER", buffer) == 0) my_register(newsd);

    if (strcmp("UNREGISTER", buffer) == 0) my_unregister(newsd);

    if (strcmp("USERS", buffer) == 0) my_users(newsd);

    if (strcmp("SEND", buffer) == 0) my_send(newsd, 0);

    if (strcmp("SENDATTACH", buffer) == 0) my_send(newsd, 1);


    
    // Cerrar la conexion y liberar el semáforo
    close(newsd);
    sem_post(&semaforo);
}



// ========================================================================
// FUNCION MAIN
// ========================================================================

int main ( int argc, char **argv )
{
    // Comprobamos que los datos pasados estén bien
    if (argc != 2) {
        printf("Uso: %s <puerto>\n", argv[0]);
        return 0;
    }

    // Cargamos la db (usuarios y mensajes) en RAM
    load_db();

    // Obtenemos el valor de la variable de entorno para RPC
    LOG_RPC_IP = getenv("LOG_RPC_IP");
    
    // DEBUG
    printf("c> LOG_RPC_IP = %s\n", LOG_RPC_IP);

    // Almacenamos el puerto de alojamiento del servidor
    int puerto = atoi(argv[1]) ;

    // Definimos las variables necesarias para iniciar el socket de peticiones del servidor
    int sd, newsd, ret;
    socklen_t size;
    struct sockaddr_in server_addr, client_addr;

    // Creamos y configuramos el socket
    sd = create_socket();
    server_addr = configure_socket("0.0.0.0", puerto);


    // Nos agenciamos el socket con la configuración
    ret = bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) ;
    if (ret < 0) {
        perror("Error en bind: ") ;
        return -1 ;
    }

    // Obtenemos en server_addr los datos del socket que acabamos de crear
    size = sizeof(struct sockaddr_in) ;
    bzero(&server_addr, size);
    getsockname(sd, (struct sockaddr *) &server_addr, &size);
    printf("s> init server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    printf("s>\n");
    fflush(stdout);

    // Abrimos el socket para poder recibir peticiones
    ret = listen(sd, SOMAXCONN);
    if (ret < 0) {
        perror("Error en listen: ") ;
        return -1 ;
    }

    // Aquí almacenaremos el hilo (detach) hasta que llegue el siguiente
    pthread_t hilo;

    sem_init(&semaforo, 0, MAX_HILOS);

    while (1)
    {
        // Separamos las peticiones 1 hueco

        // Limpiamos la basura en memoria
        bzero(&client_addr, size);
        size = sizeof(struct sockaddr_in) ;
        sem_wait(&semaforo);
        newsd = accept (sd, (struct sockaddr *) &client_addr, &size);
        if (newsd < 0) {
            perror("Error en el accept");
            return -1 ;
        }

        /*
        // <Ayuda a la depuración>
            // a) dirección rellenada por llamada accept()
            printf("conexión aceptada de IP:%s y puerto:%d\n",
                    inet_ntoa(client_addr.sin_addr),
                        ntohs(client_addr.sin_port));
            // b) dirección asociada al socket newsd en otro extremo
            char sck_IP[32] ;
            size = sizeof(struct sockaddr_in) ;
            getpeername(newsd, (struct sockaddr *)&client_addr, &size);
            inet_ntop(AF_INET, &(client_addr.sin_addr), sck_IP, sizeof(sck_IP));
            printf("conexión aceptada de IP:%s y puerto:%d\n",
                    sck_IP, ntohs(client_addr.sin_port));
        // </Ayuda a la depuración>
        */

        // Atender peticiones

        petition_t *parametro =  (petition_t *) malloc(sizeof(petition_t));
        parametro->newsd = newsd;
        inet_ntop(AF_INET, &(client_addr.sin_addr), parametro->ip, sizeof(parametro->ip));
        parametro->port = ntohs(client_addr.sin_port);

        if (pthread_create(&hilo, NULL, atender_peticion, (void *) parametro) == 0) { pthread_detach(hilo); }
        else { sem_post(&semaforo); printf("No se pudo crear la petición, la hemos perdido\n"); }
    }

    // (8) cerrar socket de servicio
    close(sd);
}