#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Iniciamos una db vacía global (para todo el programa)
user_list_t my_db = {NULL, NULL, 0, 0};

// Iniciamos un mutex global (para todo el programa)
pthread_mutex_t lock_db = PTHREAD_MUTEX_INITIALIZER;


// ========================================================================
// FUNCIONES DEBUGGING
// ========================================================================


int show_users () { // TERMINADA

    pthread_mutex_lock(&lock_db);

    // Leer el numero de usuarios
    printf("Hay %d usuarios en la bbdd\n", my_db.total_users);
    
    user_t *current = my_db.head;
    while (current != NULL) {
        printf("%s -- %s -- %d\n", current->user, current->return_ip, current->return_port);
        current = current->next;
    }

    // Actualizamos la db
    save_db ();

    pthread_mutex_unlock(&lock_db);
}


int printall () { // TERMINADA

    pthread_mutex_lock(&lock_db);

    user_t *current_user = my_db.head;
    message_t *current_message = NULL;

    while (current_user != NULL) {
        current_message = current_user->messages;
        printf("===============================\n");
        printf("%s -- %s -- %d\n\n", current_user->user, current_user->return_ip, current_user->return_port);
        while (current_message != NULL) {
            printf("\tMensaje %d de %s:\n\t%s\n\n", current_message->id, current_message->transmitter, current_message->message);
            current_message = current_message->next;
        }
        current_user = current_user->next;
    }

    pthread_mutex_unlock(&lock_db);

}


int db_test () {
    user_t *usuario = NULL;
    message_t *mensaje = NULL;

    insert_user("A");
    insert_user("B");
    insert_user("C");
    insert_user("C");
    insert_user("C");
    insert_user("D");
    insert_user("E");
    insert_user("F");

    exit(0);

    insert_message("A", 1, "CastaG", "Hola pa ti mi cola");
    insert_message("B", 2, "CastaG", "Hola pa ti mi cola");
    insert_message("C", 3, "CastaG", "Hola pa ti mi cola");
    insert_message("A", 4, "CastaG", "Hola pa ti mi cola");
    insert_message("B", 5, "CastaG", "Hola pa ti mi cola");
    insert_message("C", 6, "CastaG", "Hola pa ti mi cola");
    insert_message("A", 7, "CastaG", "Hola pa ti mi cola");
    insert_message("B", 8, "CastaG", "Hola pa ti mi cola");
    insert_message("C", 9, "CastaG", "Hola pa ti mi cola");
    insert_message("A", 10, "CastaG", "Hola pa ti mi cola");
    insert_message("B", 11, "CastaG", "Hola pa ti mi cola");
    insert_message("C", 12, "CastaG", "Hola pa ti mi cola");
    insert_message("A", 13, "CastaG", "Hola pa ti mi cola");

    printall();

    save_db();
}


// ========================================================================
// FUNCIONES EXTERNAS (SE PROTEGEN CON MUTEX SOLAS)
// ========================================================================


// 0 = OK // 1 = USER_NOT_FOUND
int insert_user (char *user) { // TERMINADA

    pthread_mutex_lock(&lock_db);

    // Verificamos si el usuario ya existe
    user_t *old = search_user(user);
    if (old != NULL) {
        pthread_mutex_unlock(&lock_db);
        return 1;
    }

    // Si no existe creamos el puntero su nodo asociado
    user_t *new = (user_t*) malloc(sizeof(user_t));
    size_t max_user_size = sizeof(new->user) - 1;


    // Le damos valores al nodo
    strncpy(new->user, user, max_user_size);
    new->user[max_user_size] = '\0';
    strcpy(new->return_ip, "");
    new->return_port = -1;
    new->connected = 0;
    new->next = NULL;
    new->prev = my_db.tail;
    new->messages = NULL;
    
    // Caso 1: Primer nodo de la lista
    if (my_db.total_users == 0) {
        my_db.head = new;
    }

    // Caso 2: Cualquier otro nodo
    else {
        my_db.tail->next = new;
    }

    // Terminamos con el código común a los caminos
    my_db.tail = new;
    my_db.total_users++;

    // Actualizamos la db
    save_db ();

    pthread_mutex_unlock(&lock_db);

    return 0;
}


// 0 = OK // 1 = USER_NOT_FOUND
int delete_user (char *user) { // TERMINADA

    pthread_mutex_lock(&lock_db);

    user_t *current = search_user(user);

    // Caso 1: El usuario no se encuentra
    if (current == NULL)  {
        pthread_mutex_unlock(&lock_db);
        return 1;
    }

    // Si se encuentra, guardamos los punteros de sus vecinos
    user_t *following = current->next;
    user_t *previous = current->prev;


    // ELIMINAR LOS MENSAJES DEL USUARIO
    
    // Tomamos el primer mensaje
    message_t *current_message = current->messages;
    message_t *next_message = NULL;

    // Mientras siga habiendo mensajes del usuario, liberamos y vamos al siguiente
    while (current_message != NULL) {
        next_message = current_message->next;
        free(current_message);
        current_message = next_message;
    }


    // ELIMINAR AL USUARIO DE LA DB

    // Si solo hay 1 usuario, no dejamos nada
    if (my_db.total_users == 1) {
        my_db.head = NULL;
        my_db.tail = NULL;
    }

    // Si el eliminado es cabecera actualizamos la etiqueta
    else if (current == my_db.head) {
        my_db.head = following;
        my_db.head->prev = NULL;
    }

    // Si el eliminado es final actualizamos la etiqueta
    else if (current == my_db.tail) {
        my_db.tail = previous;
        my_db.tail->next = NULL;
    }

    // Si es un nodo intermedio lo saltamos
    else {
        previous->next = following;
        following->prev = previous;
    }

    // En cualquier caso liberamos la memoria
    free(current);
    my_db.total_users--;

    // Actualizamos la db
    save_db ();

    pthread_mutex_unlock(&lock_db);

    return 0;
}


// 0 = OK // 1 = USER_NOT_FOUND // 2 = USER_ALREADY_CONNECTED
int connect_user (char *user, char *ip, int port) { // TERMINADA

    pthread_mutex_lock(&lock_db);

    user_t *current = search_user(user);
    
    // Si el usuario no existe retornamos 1
    if (current == NULL) {
        pthread_mutex_unlock(&lock_db);
        return 1;
    }

    if (current->connected == 1) {
        pthread_mutex_unlock(&lock_db);
        return 2;
    }

    size_t max_ip_size = sizeof(current->return_ip) - 1;

    current->connected = 1;
    strncpy(current->return_ip, ip, max_ip_size);
    current->return_ip[max_ip_size] = '\0';
    current->return_port = port;

    my_db.connected_users++;

    // Actualizamos la db
    save_db ();

    pthread_mutex_unlock(&lock_db);

    return 0;
}


// 0 = OK // 1 = USER_NOT_FOUND // 2 = USER_ALREADY_DISCONNECTED
int disconnect_user (char *user) { // TERMINADA

    pthread_mutex_lock(&lock_db);

    user_t *current = search_user(user);
    
    // Si el usuario no se encuentra retornamos 1
    if (current == NULL) {
        pthread_mutex_unlock(&lock_db);
        return 1;
    }

    // Si el cliente ya está desonectado retornamos 2
    if (current->connected == 0) {
        pthread_mutex_unlock(&lock_db);
        return 2;
    }


    current->connected = 0;
    strcpy(current->return_ip, "");
    current->return_port = -1;

    my_db.connected_users--;

    // Actualizamos la db
    save_db ();

    pthread_mutex_unlock(&lock_db);
    return 0;

}



// ========================================================================
// FUNCIONES INTERNAS (DEBEN SER PROTEGIDAS POR SUS LLAMANTES)
// ========================================================================


// ptr = OK // NULL = USER_NOT_FOUND
user_t *search_user (char *user) { // TERMINADA (protegida por sus llamantes)

    user_t *current = my_db.head;

    while (current != NULL) {
        if (strcmp(user, current->user) == 0) break;
        current = current->next;
    }

    return current;

}


// 0 = CONNECTED // 1 = DISCONNECTED // 2 = USER_NOT_FOUND
int check_user_status (char *user) { // 
    user_t *current = search_user(user);
    
    // Si el usuario no está en la db
    if (current == NULL) return 2;

    // Si el usuario está conectado a la db
    if (current->connected == 1) return 0;

    // Si el usuario está desconectado de la db
    if (current->connected == 0) return 1;

    // Otros casos (errores)
    return 2;
}


// Retorna el listado de usuarios conectados en ese momento
// NULL = 0 usuarios // ptr = 1 o más usuarios
char **return_connected () {

    // Si no hay nadie conectado
    if (my_db.connected_users == 0) return NULL;

    int pos = 0;
    user_t *current = my_db.head;
    
    char *current_ptr = NULL;
    char **lista_conectados = (char**) malloc (my_db.connected_users * sizeof(char*));

    while (current != NULL) {
        if (current->connected == 1) {
            current_ptr = (char*) malloc (MAX_STR_LEN);
            strncpy(current_ptr, current->user, MAX_STR_LEN);
            lista_conectados[pos++] = current_ptr;
        }
        current = current->next;
    }
    
    return lista_conectados;
}


// 0 = OK // 1 = USER_NOT_FOUND
int insert_message (char *user, int message_id, char *transmitter, char *message) { // TERMINADA
    
    user_t *current_user = search_user(user);
    if (current_user == NULL) {
        return 1;
    }

    // Preparamos el puntero y los tamaños maximos de las cadenas
    message_t *new = (message_t *) malloc (sizeof(message_t));
    size_t max_transmitter_size = sizeof(new->transmitter) - 1;
    size_t max_message_size = sizeof(new->message) - 1;

    // Rellenamos los datos del nuevo mensaje
    new->id = message_id;
    strncpy(new->transmitter, transmitter, max_transmitter_size);
    new->transmitter[max_transmitter_size] = '\0';
    strncpy(new->message, message, max_message_size);
    new->message[max_message_size] = '\0';
    new->next = NULL;

    // Obtenemos el comienzo de la lista de mensajes
    message_t *current_message = current_user->messages;

    // Si no hay mensajes, modificamos el puntero del user_t
    if (current_message == NULL) {
        current_user->messages = new;
    }

    // En cualquier otro caso recorremos la lista hasta un ->next que sea NULL
    else {
        while (current_message->next != NULL) {
            current_message = current_message->next;
        }
        // Encadenamos un nuevo mensaje
        current_message->next = new;
    }

    // Actualizamos la db
    save_db ();

    return 0;
}


// 0 = OK // 1 = USER_NOT_FOUND
int insert_first_message (char *user, message_t *message) { // TERMINADA
    
    user_t *current_user = search_user(user);
    if (current_user == NULL) {
        return 1;
    }

    // Obtenemos el comienzo de la lista de mensajes
    message_t *current_message = current_user->messages;

    // Si no hay mensajes, modificamos el puntero del user_t
    if (current_message == NULL) {
        current_user->messages = message;
    }

    // En cualquier otro caso recorremos la lista hasta un ->next que sea NULL
    else {
        while (current_message->next != NULL) {
            current_message = current_message->next;
        }
        // Encadenamos un nuevo mensaje
        current_message->next = message;
    }

    // Actualizamos la db
    save_db ();

    return 0;
}


// 0 = OK // 1 = USER_NOT_FOUND
int insert_last_message (char *user, message_t *new_head_message) {
    
    // Encontrar al usuario con search
    user_t *current_user = search_user(user);
    if (current_user == NULL) {
        return 1;
    }

    // Almacenamos el puntero al primer mensaje
    message_t *head_message = current_user->messages;

    // Seguro hay que actualizar al usuario
    current_user->messages = new_head_message;

    // Si hay más mensajes, el reinsertado apuntará a la antigua cabeza
    if (head_message != NULL) new_head_message->next = head_message;

    // Actualizamos la db
    save_db ();

    return 0;
}


// ptr = OK // NULL = USER_NOT_FOUND, MESSAGE_NOT_FOUND
message_t *pop_first_message (char *user) {

    // Encontrar al usuario con search
    user_t *current_user = search_user(user);
    if (current_user == NULL) {
        return NULL;
    }

    // Almacenamos el puntero al primer mensaje
    message_t *current_message = current_user->messages;
    message_t *previous_message = NULL;

    // Si no hay mensajes, no mandamos nada
    if (current_message == NULL) {
        return NULL;
    }

    // Si hay 1 mensaje, no mandamos nada
    if (current_message->next == NULL) {
        current_user->messages = NULL;
        return current_message;
    }

    // Si hay más de un mensaje, mientras no estemos en el último, avanzamos
    while (current_message->next != NULL) {
        previous_message = current_message;
        current_message = current_message->next;
    }

    // Rompemos el enlace del antepenúltimo al último
    previous_message->next = NULL;
    
    // Actualizamos la db
    save_db ();
    
    // Retornamos el último mensaje
    return current_message;
}


// ptr = OK // NULL = USER_NOT_FOUND, MESSAGE_NOT_FOUND
message_t *pop_last_message (char *user) {

    // Encontrar al usuario con search
    user_t *current_user = search_user(user);
    if (current_user == NULL) {
        return NULL;
    }

    // Almacenamos el puntero al primer mensaje
    message_t *current_message = current_user->messages;

    // Si no hay mensajes, no mandamos nada
    if (current_message == NULL) {
        return NULL;
    }

    // De lo contrario hacemos pop del mensaje
    current_user->messages = current_user->messages->next;

    // Actualizamos la db
    save_db ();
    
    return current_message;
}


int load_db () { // TERMINADA (no se usa en un momento concurrente)
    
    // Abrimos el fichero de usuarios
    FILE *fdu = fopen(DB_USERS, "r");
    if (fdu == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    // Abrimos el fichero de mensajes
    FILE *fdm = fopen(DB_MESSAGES, "r");
    if (fdm == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    // Aquí almacenamos las líneas que leamos
    char line[1024];

    // Leemos y cargamos cada usuario
    while (fgets(line, sizeof(line), fdu)) {
        
        // Extraemos el nombre de usuario
        line[strlen(line)-1] = '\0';
        if (strlen(line) == 0) continue;

        // Y lo introducimos de nuevo
        insert_user(line);
    }

    // Datos a rellenar para cada usuario
    char *user;
    int id;
    char *transmitter;
    char *message;
    
    // Leemos y cargamos cada mensaje
    while (fgets(line, sizeof(line), fdm)) {
        
        // Extraemos la línea que representa el mensaje
        line[strlen(line)-1] = '\0';
        if (strlen(line) == 0) continue;

        // Tokenizamos la línea
        user = strtok(line, "|");
        id = atoi(strtok(NULL, "|"));
        transmitter = strtok(NULL, "|");
        message = strtok(NULL, "|");

        // Introducimos el mensaje leído
        insert_message(user, id, transmitter, message);
    }
}


int save_db () { // TERMINADA (protegida por otras funciones)
    
    // Abrimos el fichero de usuarios
    FILE *fdu = fopen(DB_USERS, "w");
    if (fdu == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    // Abrimos el fichero de mensajes
    FILE *fdm = fopen(DB_MESSAGES, "w");
    if (fdm == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    // Punteros para recorrer la estructura
    user_t *current_user = my_db.head;
    message_t *current_message = NULL;

    // Por cada usuario
    while (current_user != NULL) {
        // Almacenamos el usuario
        fprintf(fdu, "%s\n", current_user->user);

        // Extraemos el primer mensaje del usuario
        current_message = current_user->messages;

        // Escribimos todos los mensajes almacenados del usuario
        while (current_message != NULL) {
            fprintf(fdm, "%s|%d|%s|%s\n", current_user->user, current_message->id, current_message->transmitter, current_message->message);
            current_message = current_message->next;
        }

        // Iteramos al siguiente
        current_user = current_user->next;
    }

    // Cerramos los ficheros
    fclose(fdm);
    fclose(fdu);
}