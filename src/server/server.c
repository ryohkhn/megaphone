
#include "../../include/server.h"

/**
 * This function initializes an array of available ports.
 */
void initialize_ports() {
    available_ports = malloc(sizeof(int) * PORT_RANGE);
    testMalloc(available_ports);
    for (uint16_t i = 0; i < PORT_RANGE; i++) {
        available_ports[i] = MIN_PORT + i;
    }
    printf("ports initialisés\n");
}

/**
 * This function allocates a port from the list of available ports.
 * @return The allocated port number.
 */
 uint16_t allocate_port() {
    int allocated_port = -1;
    pthread_mutex_lock(&port_mutex);
    for (int i = 0; i < PORT_RANGE; i++) {
        if (available_ports[i] != -1) {
            allocated_port = available_ports[i];
            available_ports[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&port_mutex);
    printf("allocated_port = %d\n", allocated_port);
    return allocated_port;
}

/**
 * This function releases a port back into the list of available ports.
 * @param port The port to be released.
 */
 void release_port(int port) {
    if (port >= MIN_PORT && port <= MAX_PORT) {
        pthread_mutex_lock(&port_mutex);
        available_ports[port - MIN_PORT] = port;
        pthread_mutex_unlock(&port_mutex);
    }
}

/**
 * This function adds a new thread into the global list of threads,
 * dynamically resizing the array as necessary.
 * @param originaire The originaire of the new thread.
 */
void add_new_fil(char* originaire) {
    // If the list is full, reallocate memory to double the capacity
    if (fils_size == fils_capacity) {
        fils_capacity *= 2;
        fils = realloc(fils, fils_capacity * sizeof(fil));
        testMalloc(fils);
    }

    (*(fils+fils_size)).fil_number=fils_size;
    (*(fils+fils_size)).last_multicasted_message = malloc(sizeof(message_node));
    testMalloc((*(fils+fils_size)).last_multicasted_message);
    (*(fils+fils_size)).subscribed = 0;
    (*(fils+fils_size)).originaire = malloc(sizeof(uint8_t)*10);
    testMalloc((*(fils+fils_size)).originaire);
    memcpy((*(fils+fils_size)).originaire,originaire,sizeof(uint8_t)*10);
    (*(fils+fils_size)).nb_messages = 0;
    (*(fils+fils_size)).addrmult = malloc(sizeof(char)*16);
    testMalloc((*(fils+fils_size)).addrmult);
    fils_size++;
}

/**
 * This function retrieves the pseudo of a client from their ID.
 * @param id The ID of the client.
 * @return The pseudo of the client.
 */
char *pseudo_from_id(int id){
    list_client *current_client=clients;
    char *pseudo="##########";
    if(current_client==NULL){
        printf("There are no clients yet!\n");
        return pseudo;
    }
    else{
        while(current_client->suivant!=NULL){
            if(current_client->id==id) break;
            current_client=current_client->suivant;
        }
        return current_client->pseudo;
    }
}

/**
 * This function serializes a message into a notification format, including the pseudo of the sender and the content of the message.
 * @param msg The message to serialize.
 * @param numfil The thread number.
 * @return A pointer to the serialized message.
 */
char *message_to_notification(message *msg,uint16_t numfil){
    size_t buffer_size = NOTIFICATION_SIZE;
    char *buffer = malloc(buffer_size);
    testMalloc(buffer);

    uint16_t entete = create_entete(SUBSCRIBE,0)->val;
    memcpy(buffer, &(entete), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t), &(numfil), sizeof(uint16_t));

    // Add pseudo
    int id = msg->id;
    char* pseudo = pseudo_from_id(id);
    memcpy(buffer + sizeof(uint16_t) * 2, pseudo, sizeof(char) * 10);

    // Add data
    uint8_t datalen = msg->datalen;
    if(datalen > 20) datalen = 20;
    memcpy(buffer + sizeof(uint16_t) * 2 + sizeof(char) * 10, msg->data, datalen);

    return buffer;
}

/**
 * This function sends a message to a client.
 * @param codereq The request type.
 * @param id The ID of the client.
 * @param nb
 * @param numfil The thread number.
 * @param sock_client The client's socket.
 */
void send_message(request_type codereq, uint16_t id, uint16_t nb, uint16_t numfil, int sock_client){
    server_message * msg = malloc(sizeof(server_message));
    testMalloc(msg);

    msg->entete.val = create_entete(codereq,id)->val;
    msg->numfil = htons(numfil);
    msg->nb = htons(nb);

    char* buffer = server_message_to_string(msg);
    ssize_t nboctet = send(sock_client, buffer, SERVER_MESSAGE_SIZE, 0);
    if(nboctet <= 0) perror("send");
}

/**
 * This function sends an error message to a client.
 * @param sock_client The client's socket.
 */
void send_error_message(int sock_client){
    send_message(ERROR,0,0,0,sock_client);
}

/**
 * This function handles the registration of a new client.
 * @param pseudo The username of the new client.
 * @param sock_client The client's socket.
 */
void inscription_client(char * pseudo, int sock_client){
    // Check if maximum number of clients has been reached
    if(id_dernier_client >= MAX_ID){
        printf("Maximum number of clients reached.\n");
        send_error_message(sock_client);
        return;
    }

    pthread_mutex_lock(&client_mutex);
    list_client * current_client = clients;
    id_dernier_client += 1;
    if(current_client == NULL){
        current_client = malloc(sizeof(list_client));
        testMalloc(current_client);
        current_client->id = id_dernier_client;
        current_client->pseudo = malloc(sizeof(char) * strlen(pseudo));
        testMalloc(current_client->pseudo);
        current_client->pseudo = pseudo;
        printf("Created first client with pseudo: %s and id: %ld\n", current_client->pseudo, current_client->id);
    }
    else {
        // on va au dernier client de la liste
        while (current_client->suivant != NULL) {
            current_client = current_client->suivant;
        }

        // on crée le suivant
        current_client->suivant = malloc(sizeof(list_client));
        testMalloc(current_client->suivant);
        current_client = current_client->suivant;

        // on modifie le suivant
        current_client->id = id_dernier_client;
        current_client->pseudo = malloc(sizeof(char) * (strlen(pseudo) + 1));
        testMalloc(current_client->pseudo);
        strncpy(current_client->pseudo, pseudo, strlen(pseudo));
        current_client->pseudo[strlen(pseudo)] = '\0';
        printf("Created client with pseudo: %s and id: %ld\n", current_client->pseudo, current_client->id);
    }
    pthread_mutex_unlock(&client_mutex);

    // on envoie le message de l'inscription
    send_message(REGISTER, current_client->id, 0, 0, sock_client);
}

void add_message_to_fil(client_message *msg, uint16_t fil_number, int isFile) {
    fil* current_fil = &fils[fil_number];
    current_fil->nb_messages++;

    message_node *new_node = malloc(sizeof(message_node));
    testMalloc(new_node);
    new_node->isFile = isFile;
    new_node->msg = malloc(sizeof(message));
    testMalloc(new_node->msg);
    new_node->msg->datalen = msg->datalen;
    new_node->msg->id = get_id_entete(msg->entete.val);

    new_node->msg->data = malloc(sizeof(uint8_t)*new_node->msg->datalen);
    testMalloc(new_node->msg->data);
    memcpy(new_node->msg->data,msg->data,new_node->msg->datalen);

    new_node->next = current_fil->head;
    current_fil->head = new_node;
}

char **retrieve_messages_from_fil(uint16_t fil_number) {
    pthread_mutex_lock(&fil_mutex);
    fil *current_fil = &fils[fil_number];

    message_node *current = current_fil->head;
    char **messages = malloc(sizeof(char *) * 1024);
    testMalloc(messages);
    int index = 0;
    while (current != NULL && current->msg != NULL) {
        messages[index] = strdup((char *)(current->msg->data));
        index++;
        current = current->next;
    }
    pthread_mutex_unlock(&fil_mutex);
    messages[index] = NULL; // Add NULL at the end of the messages array
    return messages;
}

void post_message(client_message *msg,int sock_client){
    uint16_t id = get_id_entete(msg->entete.val);

    printf("Received message from user with id %d, to post to numfil %d\n",id,ntohs(msg->numfil));
    printf("The message is: %s\n",(char *) (msg->data));

    uint16_t numfil=ntohs(msg->numfil);
    printf("Numfil reçu: %d\n",numfil);

    pthread_mutex_lock(&fil_mutex);
    if(numfil==0){
        add_new_fil(pseudo_from_id(id));
        numfil=fils_size-1;
        // pthread_mutex_unlock(&fil_mutex);
        printf("Found empty thread: %d\n",numfil);
    }
    else if(numfil>=fils_size){
        printf("Client tried to write to a nonexistent thread\n");
        pthread_mutex_unlock(&fil_mutex);
        send_message(NONEXISTENT_THREAD,0,0,0,sock_client);
        return;
    }

    add_message_to_fil(msg,numfil, 0);
    pthread_mutex_unlock(&fil_mutex);

    //TEST print all messages in the fil after adding the new one
    char **res=retrieve_messages_from_fil(numfil);
    int i=0;
    while(res[i]!=NULL){
        printf("Message %d: %s\n",i+1,res[i]);
        i++;
    }

    // Sending response to client
    send_message(POST_MESSAGE,id,0,numfil,sock_client);
}

/**
 * This function send the buffer with multiples messages if the total bytes exceed BUFSIZ.
 * Else data is appended to the buffer
 * @param buffer the buffer of size BUFSIZ to append data to
 * @param numfil the fil to copy messages from
 * @param offset the offset of the buffer to write data from
 * @param msg_nb the number of messages to copy from the fil
 * @param sock_client the socket to send data to if
 * @return the offset of the buffer where data was written
 */
size_t send_messages_from_fil(char *buffer,uint16_t numfil,size_t offset,uint16_t msg_nb,int sock_client){
    fil* current_fil=&fils[numfil];
    message_node* current_message=current_fil->head;

    for(int i=0; i<msg_nb; ++i){
        uint8_t datalen=current_message->msg->datalen;
        // We verify that the current data + the data that will be added with the current message do not exceed BUFSIZ
        // Else we send the buffer to the client
        if((offset+sizeof(uint8_t)*22+sizeof(uint8_t)*datalen)>BUFSIZ){
            ssize_t nboctet = send(sock_client, buffer,sizeof(char)*offset, 0);
            if(nboctet <= 0)perror("send n messages to client");
            printf("Octets envoyés au client: %zu\n",nboctet);

            free(buffer);
            buffer=malloc(sizeof(char)*BUFSIZ);
            testMalloc(buffer);
            memset(buffer,0,sizeof(char)*BUFSIZ);
            offset=0;
        }

        // Copy the numfil to the buffer
        uint16_t numfil_network=htons(numfil);
        memcpy(buffer+offset,&numfil_network,sizeof(uint16_t));
        offset+=sizeof(uint16_t);

        // Copy the origin of the fil to the buffer
        memcpy(buffer+offset,current_fil->originaire,sizeof(uint8_t)*10);
        offset+=sizeof(uint8_t)*10;

        // Copy the pseudo of the people that wrote the message to the buffer
        char* pseudo = pseudo_from_id(current_message->msg->id);
        memcpy(buffer+offset,pseudo,sizeof(uint8_t)*10);
        offset+=sizeof(uint8_t)*10;

        // Copy the datalen and the message to the buffer
        memcpy(buffer+offset,&datalen,sizeof(uint8_t));
        offset+=sizeof(uint8_t);
        memcpy(buffer+offset,current_message->msg->data,sizeof(uint8_t)*datalen);
        offset+=sizeof(uint8_t)*datalen;

        current_message=current_message->next;
    }
    return offset;
}

/**
 * This function handles the sending of nb messages for the fil numfil
 * @param msg the message received from the client
 * @param sock_client the socket of the current client
 */
void request_threads_list(client_message *msg, int sock_client){
    uint16_t msg_numfil=ntohs(msg->numfil);
    uint16_t msg_nb=ntohs(msg->nb);
    uint16_t nb_fil;
    // If the asked fil is 0 the server send messages from all fils
    if(msg_numfil==0){
        pthread_mutex_lock(&fil_mutex);
        uint16_t total_nb=0;
        for(uint16_t i=0; i<fils_size; ++i){
            nb_fil=fils[i].nb_messages;
            if(msg_nb==0 || msg_nb>nb_fil){
                total_nb+=nb_fil;
            }
            else{
                total_nb+=msg_nb;
            }
        }
        // Initial server message with codereq 3, same id as client, the total number of messages and the total number of fils
        send_message(LIST_MESSAGES,get_id_entete(msg->entete.val),total_nb,fils_size,sock_client);

        // The server send buffers of maximum BUFSIZ
        char* buffer = malloc(sizeof(char)*BUFSIZ);
        testMalloc(buffer);
        memset(buffer,0,sizeof(char)*BUFSIZ);
        size_t offset=0;

        for(int i=0; i<fils_size; ++i){
            nb_fil=fils[i].nb_messages;
            // skip the fil 0 if empty
            if(i==0 && nb_fil==0) continue;

            uint16_t cpy_msg_nb=msg_nb;
            if(cpy_msg_nb==0 || cpy_msg_nb>nb_fil){
                cpy_msg_nb=nb_fil;
            }

            // The server send the buffer if the total bytes exceed BUFSIZ
            // Else we get the offset of the current buffer to append more data
            offset = send_messages_from_fil(buffer,i,offset,cpy_msg_nb,sock_client);
        }
        pthread_mutex_unlock(&fil_mutex);

        // If the last buffer is not yet sent, the server send it to the client
        if(offset!=0){
            ssize_t nboctet = send(sock_client, buffer,sizeof(char)*(offset), 0);
            if(nboctet <= 0) perror("send n messages to client");
            printf("Octets envoyés au client: %zu\n",nboctet);
        }

        free(buffer);
    }
    else{
        // If the asked fil does not exist the server sends a NONEXISTENT_FIL error
        pthread_mutex_lock(&fil_mutex);
        if(msg_numfil>=fils_size){
            pthread_mutex_unlock(&fil_mutex);
            send_message(NONEXISTENT_THREAD,0,0,0,sock_client);
            return;
        }
        nb_fil=fils[msg_numfil].nb_messages;
        if(msg_nb==0 || msg_nb>nb_fil){
            msg_nb=nb_fil;
        }

        // Initial server message with codereq 3, same id as client, the total number of messages in the fil and the number of the fil
        send_message(LIST_MESSAGES,get_id_entete(msg->entete.val),msg_nb,msg_numfil,sock_client);

        // The server send buffers of maximum BUFSIZ
        char* buffer = malloc(sizeof(char)*BUFSIZ);
        testMalloc(buffer);
        memset(buffer,0,sizeof(char)*BUFSIZ);
        size_t offset=0;

        // The server send the buffer if the total bytes exceed BUFSIZ
        // Else we get the offset of the current buffer to send the correct amount of bytes to the client
        offset=send_messages_from_fil(buffer,msg_numfil,offset,msg_nb,sock_client);
        pthread_mutex_unlock(&fil_mutex);
        if(offset!=0){
            ssize_t nboctet = send(sock_client, buffer,sizeof(char)*(offset), 0);
            if(nboctet <= 0) perror("send n messages to client");
            printf("Octets envoyés au client: %zu\n",nboctet);
        }
        free(buffer);
    }
}

/**
 * This function sends a notification to all subscribers of a particular thread.
 * It creates a socket, sets up the multicast address and sends all unsent messages in the thread to the subscribers.
 *
 * @param thread_index The index of the thread in the threads array from which the messages will be sent.
 */
void send_thread_notification(uint16_t fil_index) {
    if(fils[fil_index].head == NULL) return;

    // Create a socket for sending multicast notifications
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return;
    }

    // Set the multicast address and port
    struct sockaddr_in6 dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET6, (char*) fils[fil_index].addrmult, &dest_addr.sin6_addr);

    // Initialising multicast
    int ifindex = 0;
    if(setsockopt(sockfd,IPPROTO_IPV6,IPV6_MULTICAST_IF,&ifindex,sizeof(ifindex))){
        perror("Error initialising multicast");
        pthread_mutex_unlock(&fil_mutex);
        return;
    }

    // Send the notification messages
    message_node *current_message = fils[fil_index].head;
    message_node *last_multicasted_message = fils[fil_index].last_multicasted_message;
    message_node *updated_last_multicasted_message = NULL;

    while (current_message != NULL && current_message->msg != NULL && current_message != last_multicasted_message){
        // Send the message
        printf("DEBUG In boucle %s\n", (char*) fils[fil_index].addrmult);

        char *serialized_msg =message_to_notification(current_message->msg,htons(fil_index));
        size_t serialized_msg_size = sizeof(uint8_t) * 34;

        if (sendto(sockfd, serialized_msg, serialized_msg_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto");
        }
        if(updated_last_multicasted_message == NULL){
            updated_last_multicasted_message = current_message;
        }
        current_message = current_message->next;
    }

    if(updated_last_multicasted_message != NULL){
        fils[fil_index].last_multicasted_message = updated_last_multicasted_message;
    }
    close(sockfd);
}

/**
 * This function periodically checks each thread and sends notifications for any new messages.
 * After each check, it sleeps for a predefined interval.
 */
void *send_notifications(){
    while(1){
        pthread_mutex_lock(&fil_mutex);
        for(int i=0; i<fils_size; i++){
            if(fils[i].head!=NULL && fils[i].head->msg!=NULL && fils[i].head != fils[i].last_multicasted_message
            && fils[i].subscribed>0){
                // Send notifications for the thread
                printf("Sending thread notifification to thread %d subscribers.\n",i);
                send_thread_notification(i);
            }
        }
        pthread_mutex_unlock(&fil_mutex);
        sleep(NOTIFICATION_INTERVAL);
    }
    return NULL;
}

/**
 * This function adds a subscription for a client to a specific fil.
 * It checks whether the thread already has a multicast address or not. If not, it prepares a new one.
 * Then it locks the thread_mutex to safely add the client's subscription to the thread and increases the subscription counter.
 * If the thread does not exist, it informs the client about the error.
 *
 * @param received_msg A pointer to the client_message structure which includes the thread number the client wants to subscribe to.
 * @param sock_client The client's socket which is used to send messages back to the client.
 */
void add_subscription_to_thread(client_message *received_msg, int sock_client){
    uint16_t numfil = ntohs(received_msg->numfil);
    int id = ntohs(received_msg->entete.val)>>5;

    // Prepare the multicast address: start at base and add numfil
    const char *base_address = MULTICAST_BASE_ADDRESS;
    struct in6_addr multicast_address;

    if(inet_pton(AF_INET6,base_address,&multicast_address)<=0){
        perror("inet_pton");
        return;
    }

    // Increment the last 32 bits by the numfil in order to have a unique multicast address
    uint32_t *derniers_4_octets = (uint32_t *) (&multicast_address.s6_addr32[3]);
    *derniers_4_octets = htonl(ntohl(*derniers_4_octets)+numfil);

    char address_str[INET6_ADDRSTRLEN];
    if(inet_ntop(AF_INET6,&multicast_address,address_str,INET6_ADDRSTRLEN)==NULL){
        perror("inet_ntop");
        return;
    }

    pthread_mutex_lock(&fil_mutex);
    // Check if fil already has a multicast address
    if(numfil >= fils_size || numfil == 0){
        printf("Client tried to subscribe to a nonexistent fil\n");
        send_message(NONEXISTENT_THREAD,0,0,0,sock_client);
        server_subscription_message *msg=malloc(sizeof(server_subscription_message));
        testMalloc(msg);
        msg->entete.val=create_entete(NONEXISTENT_THREAD,0)->val;
        msg->addrmult=malloc(sizeof(uint8_t)*16);
        testMalloc(msg->addrmult);

        // Serialize the server_subscription structure and send to clientfd
        char *serialized_msg=server_subscription_message_to_string(msg);
        size_t serialized_msg_size=sizeof(uint8_t)*22;

        ssize_t nboctet=send(sock_client,serialized_msg,serialized_msg_size,0);
        if(nboctet<=0)perror("send");
        pthread_mutex_unlock(&fil_mutex);
        return;
    }
    if(strlen(fils[numfil].addrmult)>0){
        printf("Multicast for this fil is already initialised.\n");
    }
    else{
        // Allocate memory for fils[numfil]->addrmult if it's not already allocated
        if(fils[numfil].addrmult==NULL){
            fils[numfil].addrmult=malloc(sizeof(uint8_t)*16);
            testMalloc(fils[numfil].addrmult);
        }

        // Copy the content of addresse_a_envoyer into fils[numfil]->addrmult
        memcpy(fils[numfil].addrmult,address_str,sizeof(uint8_t)*16);
    }

    // update fil subscription counter
    fils[numfil].subscribed+=1;
    pthread_mutex_unlock(&fil_mutex);

    //sending response to client
    server_subscription_message *msg=malloc(sizeof(server_subscription_message));
    testMalloc(msg);
    msg->entete.val=create_entete(SUBSCRIBE,id)->val;
    msg->numfil=htons(numfil);
    msg->nb=htons(MULTICAST_PORT);
    msg->addrmult=malloc(sizeof(uint8_t)*16);
    testMalloc(msg->addrmult);
    memcpy(msg->addrmult,address_str,sizeof(uint8_t)*16);

    for(int i=0; i<16; i++){
        for(int j=7; j>=0; j--){
            printf("%d",(msg->addrmult[i]>>j) & 1);
        }
        printf("%s",(i<15)?":":"\n");
    }

    // Serialize the server_subscription structure and send to clientfd
    char *serialized_msg=server_subscription_message_to_string(msg);
    size_t serialized_msg_size=sizeof(uint8_t)*22;

    ssize_t nboctet=send(sock_client,serialized_msg,serialized_msg_size,0);
    printf("DEBUG SENT %zd OCTETS\n",nboctet);
    if(nboctet<=0)perror("send");
}

void add_file(client_message *received_msg, int sock_client) {
    printf("\n\ndébut add_file\n\n");
    printf("received_msg->entete.val = %d\n", received_msg->entete.val);
    printf("received_msg->nb = %d\n", received_msg->nb);
    printf("received_msg->numfil = %d\n", received_msg->numfil);
    printf("received_msg->datalen = %d\n",received_msg->datalen);
    printf("received_msg->data = %s\n", received_msg->data);
    received_msg->entete.val = ntohs(received_msg->entete.val);
    received_msg->numfil = ntohs(received_msg->numfil);
    received_msg->nb = ntohs(received_msg->nb);
    printf("\napres ntohs() :\n");
    printf("received_msg->numfil = %d\n", received_msg->numfil);
    printf("received_msg->datalen = %d\n",received_msg->datalen);
    printf("received_msg->data = %s\n", received_msg->data);

    // on fait les vérifications de pour ajouter un billet à un fil
    pthread_mutex_lock(&fil_mutex);
    if (received_msg->numfil == 0) {
        add_new_fil(pseudo_from_id(received_msg->entete.val >> 5));
        received_msg->numfil = fils_size - 1;
        printf("Numfil trouvé vide, création d'un nouveau fil : %d\n", received_msg->numfil);
    }
    else if (received_msg->numfil >= fils_size) {
        printf("Client tried to write to a nonexistent fil\n");
        pthread_mutex_unlock(&fil_mutex);
        send_message(NONEXISTENT_THREAD,0,0,0,sock_client);
        return;
    }
    pthread_mutex_unlock(&fil_mutex);

    printf("allocate port\n");
    int port = allocate_port();

    printf("Envoi du message avec le port au client\n");
    // envoie message avec le port au client
    send_message(UPLOAD_FILE, received_msg->entete.val >> 5, port, received_msg->numfil, sock_client);

    // on reçoit et on écrit le fichier
    receive_file_udp(directory_for_files, port, received_msg->numfil, (char *)received_msg->data);

    printf("fin while, release_port, fclose, close\n");

    release_port(port);

    // on ajoute le fichier au fil
    printf("on ajoute le fichier au fil\n");
    pthread_mutex_lock(&fil_mutex);
    add_message_to_fil(received_msg,received_msg->numfil, 1);
    pthread_mutex_unlock(&fil_mutex);
}

void download_file(client_message *received_msg, int sockclient, char * client_IP) {
    printf("message initial\n");
    printf("received_msg->entete.val = %d\n", received_msg->entete.val);
    printf("received_msg->nb = %d\n",received_msg->nb);
    printf("received_msg->numfil = %d\n",received_msg->numfil);
    printf("received_msg->datalen = %d\n",received_msg->datalen);
    printf("received_msg->data = %s\n", received_msg->data);
    received_msg->numfil = ntohs(received_msg->numfil);
    received_msg->nb = ntohs(received_msg->nb);
    received_msg->entete.val = ntohs(received_msg->entete.val);
    printf("\napres ntohs() :\n");
    printf("received_msg->numfil = %d\n", received_msg->numfil);
    printf("received_msg->datalen = %d\n",received_msg->datalen);
    printf("received_msg->data = %s\n", received_msg->data);

    // on fait les vérifications de pour ajouter un billet à un fil
    pthread_mutex_lock(&fil_mutex);
    if (received_msg->numfil == 0 || received_msg->numfil >= fils_size) {
        printf("error 1\n");
        printf("Client tried to read to a nonexistent fil\n");
        pthread_mutex_unlock(&fil_mutex);
        send_message(NONEXISTENT_THREAD,0,0,0,sockclient);
        return;
    }
    pthread_mutex_unlock(&fil_mutex);
    printf("downloaded_files\n");
    printf("\n\nenvoie 1ere réponse au client:\n");
    printf("received_msg->entete.val = %d\n", received_msg->entete.val);
    printf("received_msg->nb = %d\n",received_msg->nb);
    printf("received_msg->numfil = %d\n",received_msg->numfil);

    printf("\n\n on ouvre le fichier demandé\n");
    //on ouvre le fichier demandé (stocké dans fichiers_fil)
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%d_%s", directory_for_files, received_msg->numfil, received_msg->data);
    printf("file_path = %s\n", file_path);
    if(access(file_path, F_OK) != 0) {   // F_OK vérifie l'existence du fichier
        printf("Le fichier n'existe pas.\n");
        send_message(NONEXISTENT_FILE,(received_msg->entete.val)>>5, received_msg->nb, received_msg->numfil, sockclient);
        return;
    }

    // réponse du serveur avec CODEREQ, ID et NUMFIL et NB ayant chacun la même valeur que dans
    // la requête du client pour signifier qu’il accepte la requête et va procéder au transfert.
    send_message(DOWNLOAD_FILE,(received_msg->entete.val)>>5, received_msg->nb, received_msg->numfil, sockclient);

    FILE * file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Erreur d'ouverture du fichier");
        return;
    }

    printf("\n\nappel boucle envoie udp \n\n");
    // appelle a la boucle qui envoie le message en UDP
    // arguments -> le FILE, le port, le message du client initial (pour l'entete UNIQUEMENT)
    send_file_udp(file,received_msg->nb, received_msg, client_IP);

    // nettoyage et free
    fclose(file);
    printf("\n\nfin download_files\n");
}

int verify_user_id(uint16_t id){
    pthread_mutex_lock(&client_mutex);
    if(id !=0 && id > id_dernier_client){
        pthread_mutex_unlock(&client_mutex);
        return 0;
    }
    pthread_mutex_unlock(&client_mutex);
    return 1;
}

void *serve(void *arg){
    thread_args *args = (thread_args *)arg;
    int sock_client = *(args->sock_client);
    char * client_ip = args->client_ip;

    char *buffer = malloc(HEADER_SIZE);
    testMalloc(buffer);

    // We first get the header sent by the client
    ssize_t read = recv_bytes(sock_client,buffer,HEADER_SIZE);
    printf("Octets reçus: %zd\n",read);
    if(read<0){
        perror("recv server first header");
        exit(1);
    }
    else if(read==0){
        printf("Connection with server closed");
        close(sock_client);
    }

    // We get the value of the header
    uint16_t header;
    memcpy(&header, buffer, HEADER_SIZE);
    free(buffer);

    uint16_t id_client = get_id_entete(header);
    if(!verify_user_id(id_client)){
        printf("A user tried to connect with a non-existent ID\n");
        send_message(NONEXISTENT_ID,0,0,0,sock_client);
        return NULL;
    }

    request_type codereq = get_codereq_entete(header);
    printf("CODEREQ %d\n",codereq);

    if(codereq == REGISTER){
        buffer = malloc(PSEUDO_SIZE);
        testMalloc(buffer);

        read = recv_bytes(sock_client,buffer,PSEUDO_SIZE);
        if(read<0){
            perror("recv server pseudo register");
            exit(1);
        }
        else if(read==0){
            close(sock_client);
        }

        inscription *insc = malloc(sizeof(inscription));
        memcpy(&insc->pseudo,buffer,PSEUDO_SIZE);
        free(buffer);

        inscription_client(insc->pseudo,sock_client);
    }
    else{
        ssize_t len = CLIENT_MESSAGE_SIZE + DATALEN_SIZE - HEADER_SIZE;
        buffer = malloc(len);
        read = recv_bytes(sock_client,buffer,len);
        printf("Octets reçus: %zd\n",read);
        if(read<0){
            perror("recv client message in server");
            exit(1);
        }
        else if(read==0){
            close(sock_client);
        }

        client_message *received_msg = string_to_client_message(buffer);
        received_msg->entete.val=header;
        uint8_t datalen = received_msg->datalen;
        printf("Datalen: %d\n",datalen);

        if(datalen != 0){
            buffer = malloc(sizeof(char)*datalen);
            read = recv_bytes(sock_client,buffer,datalen);
            printf("Octets reçus: %zd\n",read);
            if(read<0){
                perror("recv data from client");
                exit(1);
            }
            else if(read==0){
                close(sock_client);
            }

            received_msg->data = malloc(sizeof(uint8_t) * datalen);
            testMalloc(received_msg->data);

            // Copy the data from the buffer, starting after datalen
            memcpy(received_msg->data, buffer, sizeof(uint8_t)*datalen);
        }

        uint16_t nb = ntohs(received_msg->nb);

        switch(codereq){
            case POST_MESSAGE:
                if(nb!=0)
                    send_error_message(sock_client);
                else
                    post_message(received_msg,sock_client);
                break;
            case LIST_MESSAGES:
                if(received_msg->datalen!=0)
                    send_error_message(sock_client);
                else
                    request_threads_list(received_msg,sock_client);
                break;
            case SUBSCRIBE:
                printf("User wants to join fil %d\n",ntohs(received_msg->numfil));
                add_subscription_to_thread(received_msg,sock_client);
                break;
            case UPLOAD_FILE:
                if(nb!=0)
                    send_error_message(sock_client);
                else
                    add_file(received_msg,sock_client);
                break;
            case DOWNLOAD_FILE:
                download_file(received_msg,sock_client,client_ip);
                printf("on quitte le download file\n");
                break;
            default:
                perror("Server codereq selection");
                send_error_message(sock_client);
                break;
        }
    }

    ssize_t ret = close(sock_client);
    if(ret<0){
        perror("Close client's socket in thread");
        exit(1);
    }

    return NULL;
}


int main(int argc, char** argv){
    if(argc == 1){
        server_addr = LOCAL_ADDR;
        server_port = PORT;
    }
    else if (argc != 3) {
        printf("Usage: %s <IP Address> <Port>\nOr: %s to launch locally on the port %d\n", argv[0], argv[0], PORT);
        exit(EXIT_FAILURE);
    }
    else{
        server_addr = argv[1];
        server_port = atoi(argv[2]);
        if(server_port < 1024 ){
            printf("Please enter a valid port in the range [1024-65535]\n");
            exit(EXIT_FAILURE);
        }
    }

    //*** creation de l'adresse du destinataire (serveur) ***
    struct sockaddr_in6 address_sock;
    memset(&address_sock,0,sizeof(address_sock));
    address_sock.sin6_family = AF_INET6;
    address_sock.sin6_port = htons(server_port);
    if (inet_pton(AF_INET6, server_addr, &(address_sock.sin6_addr)) <= 0) {
        printf("Unknown address %s.\n Please make sure the address is IPv6 format.\n", server_addr);
        exit(EXIT_FAILURE);
    }

    //*** creation de la socket ***
    int sock = socket(PF_INET6,SOCK_STREAM,0);
    if(sock < 0){
        perror("creation socket");
        exit(EXIT_FAILURE);
    }

    //*** desactiver l'option n'accepter que de l'IPv6 **
    int optval = 0;
    int r_opt = setsockopt(sock,IPPROTO_IPV6,IPV6_V6ONLY,&optval,sizeof(optval));
    if(r_opt < 0) perror("Error while allowing IPv4");

    //*** le numero de port peut etre utilise en parallele ***
    optval = 1;
    r_opt = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(r_opt < 0) perror("Error SO_REUSEADDR");

    //*** on lie la socket au port ***
    int r = bind(sock,(struct sockaddr *) &address_sock,sizeof(address_sock));
    if(r < 0){
        perror("Bind error");
        exit(EXIT_FAILURE);
    }

    //*** Le serveur est pret a ecouter les connexions sur le port ***
    r = listen(sock,0);
    if(r < 0){
        perror("Listen error");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("Error initialising sigaction");
        exit(EXIT_FAILURE);
    }

    //initialize the ports list
    initialize_ports();

    // Initialise the fils
    fils = malloc(sizeof(fil));
    testMalloc(fils);

    // The thread 0 has no originaire
    add_new_fil("");

    // Initialise the client list
    clients = malloc(sizeof(list_client));
    testMalloc(clients);

    // Start the notification thread
    pthread_t notification_thread;
    pthread_create(&notification_thread, NULL, send_notifications, NULL);

    // create the directory for added files
    int result = mkdir(directory_for_files, 0777);
    if (result == 0) {
        printf("Folder \"%s\" has been successfully created.\n", directory_for_files);
    } else if (errno == EEXIST) {
        printf("The folder \"%s\" already exists.\n", directory_for_files);
    } else {
        perror("Error creating folder.");
    }


    while(running){
        struct sockaddr_in6 addrclient;
        socklen_t size = sizeof(addrclient);

        //*** we allocate the socket ***
        int *sock_client = malloc(sizeof(int));
        testMalloc(sock_client);

        //*** the server accepts a connection ***
        *sock_client = accept(sock,(struct sockaddr *) &addrclient,&size);

        char client_ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(addrclient.sin6_addr), client_ip, INET6_ADDRSTRLEN);

        //*** we prepare a structure with the thread arguments
        thread_args *args = malloc(sizeof(thread_args));
        testMalloc(args);
        args->sock_client = sock_client;
        strcpy(args->client_ip, client_ip);

        if(sock_client>=0){
            pthread_t thread;
            //*** the server creates a thread ***
            if(pthread_create(&thread,NULL,serve,args)==-1){
                perror("pthread_create");
                continue;
            }
            //*** printing the adress of connected client ***
            char nom_dst[INET6_ADDRSTRLEN];
            printf("New client connection : %s %d\n",inet_ntop(AF_INET6,&addrclient.sin6_addr,nom_dst,sizeof(nom_dst)), htons(addrclient.sin6_port));
        }
        else free(sock_client);
    }
    pthread_cancel(notification_thread);


    //*** Closing server socket ***
    printf("Closing socket\n");
    close(sock);

    free(clients);
    free(fils);
    return 0;
}
