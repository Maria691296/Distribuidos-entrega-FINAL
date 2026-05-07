char** tokenizar_mensaje (char *mensaje, char *separador) {
    int ntoken = 0;
    char *token;
    char **tokens = (char**) malloc ((MAX_TOKENS + 1)*sizeof(char*));

    token = strtok(mensaje, separador);

    while(token != NULL && ntoken < MAX_TOKENS) {
        tokens[ntoken++] = token;
        token = strtok(NULL, separador);
    }
    return tokens;
}


    /*
    if (strcmp("CONNECT", buffer) == 0) {
        // Recibimos la cadena que representa al usuario
        bzero(buffer, sizeof(buffer));
        readMessage(newsd, buffer);
        printf("Connecting user %s to the service...\n", buffer);
        strcpy(user, buffer);

        // Recibimos el puerto del client-side-server
        bzero(buffer, sizeof(buffer));
        readMessage(newsd, buffer);
        printf("Registered on %s::%s\n", peer_ip, buffer);
        listen_port = atoi(buffer);

        // Ejecutamos la función real sobre la db
        codigo_respuesta = connect_user(user, peer_ip, listen_port);
        
        // Mandamos el código de respuesta de vuelta
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "%d", codigo_respuesta);
        sendMessage(newsd, buffer, strlen(buffer)+1);
    }
    */

    /*
    if (strcmp("DISCONNECT", buffer) == 0) {
        // Recibimos la cadena que representa al usuario
        bzero(buffer, sizeof(buffer));
        readMessage(newsd, buffer);
        printf("Disconnecting user %s from the service...\n", buffer);
        strcpy(user, buffer);

        // Ejecutamos la funcionalidad real sobre la db
        codigo_respuesta = disconnect_user(user);

        // Mandamos el código de respuesta
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "%d", codigo_respuesta);
        sendMessage(newsd, buffer, strlen(buffer)+1);
    }
    */

    /*
    if (strcmp("REGISTER", buffer) == 0) {
        // Recibimos la cadena que representa al usuario
        bzero(buffer, sizeof(buffer));
        readMessage(newsd, buffer);
        printf("Attemping to register user %s on the data base...\n", buffer);
        strcpy(user, buffer);

        // Ejecutamos el código real de la db
        codigo_respuesta = insert_user(user);

        // Mandamos el código de operación
        sprintf(buffer, "%d", codigo_respuesta);
        sendMessage(newsd, buffer, strlen(buffer)+1);
    }
    */

    /*
    if (strcmp("UNREGISTER", buffer) == 0) {
        // Recibimos la cadena que representa al usuario
        bzero(buffer, sizeof(buffer));
        readMessage(newsd, buffer);
        printf("Attemping to unregister user %s from the data base...\n", buffer);
        strcpy(user, buffer);

        // Ejecutamos el código real de la db
        codigo_respuesta = delete_user(user);
        
        // Mandamos el código de operación
        sprintf(buffer, "%d", codigo_respuesta);
        sendMessage(newsd, buffer, strlen(buffer)+1);
    }
    */

    /*
    if (strcmp("USERS", buffer) == 0) {
        // Adquirimos el mutex para evitar problemas de condiciones de carrera en la db
        pthread_mutex_lock(&lock_db);
        
        // Recibimos la cadena que representa al usuario
        bzero(buffer, sizeof(buffer));
        readMessage(newsd, buffer);
        printf("Attemping to list all connected users for user %s...\n", buffer);
        strcpy(user, buffer);

        // Verificamos el estado de conexion del usuario 0 = CONECTADO, 1 = DESCONECTADO, 2 = NO EXISTE
        int connection_status = is_connected(user);

        // DEBUGGING
        // printf("CODIGO DE is_connected %d\n", connection_status);

        // Mandamos el código de respuesta al cliente para ponerse de acuerdo
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "%d", connection_status);
        sendMessage(newsd, buffer, strlen(buffer)+1); // 2 = 1 byte de respuesta + '\0'

        // Si todo ha ido bien, ejecutamos el siguiente bloque
        if (connection_status == 0) {

            // Obtenemos la lista de usuarios conectados
            char **lista = return_connected();

            // Avisamos al cliente de los mensajes que va a recibir
            bzero(buffer, sizeof(buffer));
            sprintf(buffer, "%d", my_db.connected_users);
            sendMessage(newsd, buffer, strlen(buffer)+1);

            // Y le mandamos cada usuario por separado
            for (int i = 0; i < my_db.connected_users; i++) {
                bzero(buffer, sizeof(buffer));
                sprintf(buffer, "   %s", lista[i]);
                sendMessage(newsd, buffer, strlen(buffer)+1);
                free(lista[i]);
            }

            // Liberamos el espacio reservado al array de punteros
            free(lista);
            
            }
        
        pthread_mutex_unlock(&lock_db);
    }
    */