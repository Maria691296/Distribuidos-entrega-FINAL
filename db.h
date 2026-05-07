#ifndef DB
#define DB

#include <pthread.h>

#define DB_USERS "db_users.txt"
#define DB_MESSAGES "db_messages.txt"
#define MAX_STR_LEN 256

typedef struct message_node {
    // Datos del mensaje
    int id;
    char transmitter[256];
    char message[256];
    // Puntero al siguiente mensaje
    struct message_node *next;
} message_t;

typedef struct user_node {
    // Datos del usuario
    char user[256];
    char return_ip[16];
    int return_port;
    int connected;
    // Punteros los usuarios vecinos
    struct user_node *next;
    struct user_node *prev;
    // Puntero a los mensajes pendientes del usuario
    message_t *messages;
} user_t;

typedef struct {
    user_t *head;
    user_t *tail;
    int total_users;
    int connected_users;
} user_list_t;

// Variables "singleton"
extern user_list_t my_db; 
extern pthread_mutex_t lock_db;

// Funciones de usuario
user_t *search_user (char * user);
int show_users ();
int insert_user (char *user);
int delete_user (char *user);
int connect_user (char *user, char *ip, int port);
int disconnect_user (char *user);

int check_user_status (char *user);
char **return_connected ();

// Funciones de mensaje
int insert_message (char *user, int message_id, char *transmitter, char *message);
// 0 = OK // 1 = USER_NOT_FOUND
int insert_first_message (char *user, message_t *message);
int insert_last_message (char *user, message_t *new_head_message);
message_t *pop_first_message (char *user);
message_t *pop_last_message (char *user);
int delete_messages (char * user);

// Funciones de ficheros
int load_db ();
int save_db();

#endif