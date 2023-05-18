#include "../../include/server.h"


#define PORT 7777
#define SIZE_BUF 256

#define MIN_PORT 49152
#define MAX_PORT 65535
#define PORT_RANGE (MAX_PORT - MIN_PORT + 1)

// Liste des ports disponibles
uint16_t available_ports[PORT_RANGE];

// Mutex pour la gestion des ports
pthread_mutex_t port_mutex = PTHREAD_MUTEX_INITIALIZER;

// Fonction pour initialiser la liste des ports disponibles
void initialize_ports() {
    for (uint16_t i = 0; i < PORT_RANGE; i++) {
        available_ports[i] = MIN_PORT + i;
    }
}

// Fonction pour allouer un port
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

// Fonction pour libérer un port
void release_port(int port) {
    if (port >= MIN_PORT && port <= MAX_PORT) {
        pthread_mutex_lock(&port_mutex);
        available_ports[port - MIN_PORT] = port;
        pthread_mutex_unlock(&port_mutex);
    }
}

void add_new_fil(char* originaire) {
    // If the list is full, reallocate memory to double the capacity
    if (fils_size == fils_capacity) {
        fils_capacity *= 2;
        fils = realloc(fils, fils_capacity * sizeof(fil));
        testMalloc(fils);
    }

    (*(fils+fils_size)).fil_number=fils_size;
    //(*(fils+fils_size)).head=malloc(sizeof(message_node));
    (*(fils+fils_size)).last_multicasted_message=malloc(sizeof(message_node));
    (*(fils+fils_size)).subscribed=0;
    (*(fils+fils_size)).originaire=malloc(sizeof(uint8_t)*10);
    memcpy((*(fils+fils_size)).originaire,originaire,sizeof(uint8_t)*10);
    (*(fils+fils_size)).nb_messages=0;
    (*(fils+fils_size)).addrmult=malloc(sizeof(char)*16);
    fils_size++;
}

char *pseudo_from_id(int id){
    list_client *current_client=clients;
    char *pseudo="##########";
    if(current_client==NULL){
        printf("Pas de clients!\n");
        return pseudo;
    }else{
        while(current_client->suivant!=NULL){
            if(current_client->id==id) break;
            current_client=current_client->suivant;
        }
        printf("Pseudo = %s\n",current_client->pseudo);
        return current_client->pseudo;
    }
}

char *message_to_notification(message *msg,uint16_t numfil){
    size_t buffer_size = sizeof(uint8_t) * 34;
    char *buffer = malloc(buffer_size);

    uint16_t entete=create_entete(SUBSCRIBE,0)->val;

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

void send_message(request_type codereq, uint16_t id, uint16_t nb, uint16_t numfil, int sock_client){
    // TODO SERIALIZE EN STRING
    server_message * msg = malloc(sizeof(server_message));

    msg->entete.val = create_entete(codereq,id)->val;
    msg->numfil = htons(numfil);
    msg->nb = htons(nb);

    ssize_t nboctet = send(sock_client, msg, sizeof(server_message), 0);
    if(nboctet <= 0)perror("send");
}

void send_error_message(int sock_client){
    send_message(31,0,0,0,sock_client);
}

// TODO mutex
void inscription_client(char * pseudo, int sock_client){
    list_client * current_client = clients;
    id_dernier_client += 1;
    if(current_client == NULL){
        current_client = malloc(sizeof(list_client));
        current_client->id = id_dernier_client;
        current_client->pseudo = malloc(sizeof(char) * strlen(pseudo));
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
        current_client = current_client->suivant;

        // on modifie le suivant
        current_client->id = id_dernier_client;
        current_client->pseudo = malloc(sizeof(char) * (strlen(pseudo) + 1));
        strncpy(current_client->pseudo, pseudo, strlen(pseudo));
        current_client->pseudo[strlen(pseudo)] = '\0';
        printf("Created client with pseudo: %s and id: %ld\n", current_client->pseudo, current_client->id);

    }

    // on envoie le message de l'inscription
    send_message(1, current_client->id, 0, 0, sock_client);
}

void add_message_to_fil(client_message *msg, uint16_t fil_number) {
    fil* current_fil = &fils[fil_number];
    current_fil->nb_messages++;

    message_node *new_node = malloc(sizeof(message_node));
    new_node->msg = malloc(sizeof(message));
    new_node->msg->datalen = msg->datalen;
    new_node->msg->id = get_id_entete(msg->entete.val);

    new_node->msg->data = malloc(sizeof(uint8_t)*new_node->msg->datalen);
    memcpy(new_node->msg->data,msg->data,new_node->msg->datalen);

    new_node->next = current_fil->head;
    current_fil->head = new_node;
}

char** retrieve_messages_from_fil(uint16_t fil_number) {
    fil* current_fil = &fils[fil_number];

    message_node *current = current_fil->head;
    char **messages = malloc(sizeof(char *) * 1024);
    int index = 0;
    while (current != NULL && current->msg != NULL) {
        messages[index] = strdup((char *)(current->msg->data));
        index++;
        current = current->next;
    }
    messages[index] = NULL; // Add NULL at the end of the messages array
    return messages;
}

void poster_billet(client_message *msg,int sock_client){
    uint16_t id=get_id_entete(msg->entete.val);

    printf("Received message from user with id %d, to post to numfil %d\n",id,ntohs(msg->numfil));
    printf("The message is: %s\n",(char *) (msg->data));

    uint16_t numfil=ntohs(msg->numfil);
    printf("Numfil reçu: %d\n",numfil);

    if(numfil==0){
        add_new_fil(pseudo_from_id(id));
        numfil=fils_size-1;
        printf("Numfil trouvé vide: %d\n",numfil);
    }
    else if(numfil>=fils_size){
        printf("Demande d'écriture sur un fil inexistant\n");
        numfil=0;
    }

    add_message_to_fil(msg,numfil);

    //TEST print all messages in the fil after adding the new one
    char **res=retrieve_messages_from_fil(numfil);
    int i=0;
    while(res[i]!=NULL){
        printf("Message %d: %s\n",i+1,res[i]);
        i++;
    }

    // Sending response to client
    send_message(2,id,0,numfil,sock_client);
}

/**
 * This function send the buffer with multiples messages if the total bytes exceed BUFSIZ
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
            if(nboctet <= 0)perror("send n billets to client");
            printf("Octets envoyés au client: %zu\n",nboctet);

            free(buffer);
            buffer=malloc(sizeof(char)*BUFSIZ);
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
void demander_liste_billets(client_message *msg, int sock_client){
    uint16_t msg_numfil=ntohs(msg->numfil);
    uint16_t msg_nb=ntohs(msg->nb);
    uint16_t nb_fil;
    // If the asked fil is 0 the server send messages from all fils
    if(msg_numfil==0){
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
        char* buffer=malloc(sizeof(char)*BUFSIZ);
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
            offset=send_messages_from_fil(buffer,i,offset,cpy_msg_nb,sock_client);
        }

        // If the last buffer is not yet sent, the server send it to the client
        if(offset!=0){
            ssize_t nboctet = send(sock_client, buffer,sizeof(char)*(offset), 0);
            if(nboctet <= 0) perror("send n billets to client");
            printf("Octets envoyés au client: %zu\n",nboctet);
        }

        free(buffer);
    }
    else{
        // If the asked fil does not exist the server send a 31 code
        if(msg_numfil>=fils_size){
            send_error_message(sock_client);
            return;
        }
        nb_fil=fils[msg_numfil].nb_messages;
        if(msg_nb==0 || msg_nb>nb_fil){
            msg_nb=nb_fil;
        }

        // Initial server message with codereq 3, same id as client, the total number of messages in the fil and the number of the fil
        send_message(LIST_MESSAGES,get_id_entete(msg->entete.val),msg_nb,msg_numfil,sock_client);

        // The server send buffers of maximum BUFSIZ
        char* buffer=malloc(sizeof(char)*BUFSIZ);
        memset(buffer,0,sizeof(char)*BUFSIZ);
        size_t offset=0;

        // The server send the buffer if the total bytes exceed BUFSIZ
        // Else we get the offset of the current buffer to send the correct amount of bytes to the client
        offset=send_messages_from_fil(buffer,msg_numfil,offset,msg_nb,sock_client);

        if(offset!=0){
            ssize_t nboctet = send(sock_client, buffer,sizeof(char)*(offset), 0);
            if(nboctet <= 0) perror("send n billets to client");
            printf("Octets envoyés au client: %zu\n",nboctet);
        }

        free(buffer);
    }
}


void send_fil_notification(uint16_t fil_index) {
    if(fils[fil_index].head == NULL) return;

    // Create a socket for sending multicast notifications
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the multicast address and port
    struct sockaddr_in6 dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET6, (char*) fils[fil_index].addrmult, &dest_addr.sin6_addr);

    // Send the notification messages
    message_node *current_message = fils[fil_index].head;
    message_node *last_multicasted_message = fils[fil_index].last_multicasted_message;
    message_node *updated_last_multicasted_message = NULL;

    while (current_message != NULL && current_message->msg != NULL && current_message != last_multicasted_message){
        // Send the message
        printf("DEBUG In boucle\n");

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
    //printf("New last multicasted message: %c\n\n",*(fils[fil_index].last_multicasted_message->msg->data));
    if(updated_last_multicasted_message != NULL){
        fils[fil_index].last_multicasted_message = updated_last_multicasted_message;
    }
    close(sockfd);
}

// Fonction qui vérifie un par un les fils, si il y a besoin d'envoyer
// des messages (par exemple il y a au moins un abonné et il y a au moins un message),
// on lance la fonction send_fil_notification
void *send_notifications(void *arg){
    while(1){
        for(int i=0; i<fils_size; i++){
            if( fils[i].head!=NULL && fils[i].head->msg!=NULL && fils[i].head != fils[i].last_multicasted_message
            && fils[i].subscribed>0){
                // Send notifications for the fil
                printf("Sending fil notif to fil %d\n",i);
                send_fil_notification(i);
            }
        }
        sleep(NOTIFICATION_INTERVAL);
    }
    return NULL;
}


void add_subscription_to_fil(client_message *received_msg, int sock_client){
    uint16_t numfil=ntohs(received_msg->numfil);
    int id=ntohs(received_msg->entete.val)>>5;

    // Prepare the multicast address: start at base and add numfil
    const char *base_address="ff02::1:1";
    struct in6_addr multicast_address;

    if(inet_pton(AF_INET6,base_address,&multicast_address)<=0){
        perror("inet_pton");
        return;
    }

    // Increment the last 32 bits by the numfil
    uint32_t *derniers_4_octets=(uint32_t *) (&multicast_address.s6_addr32[3]);
    *derniers_4_octets=htonl(ntohl(*derniers_4_octets)+numfil);

    char address_str[INET6_ADDRSTRLEN];
    if(inet_ntop(AF_INET6,&multicast_address,address_str,INET6_ADDRSTRLEN)==NULL){
        perror("inet_ntop");
        return;
    }

    uint8_t *addresse_a_envoyer;

    // Check if fil already has a multicast address
    if(numfil >= fils_size){
      printf("%d CODEREQ 30\n", fils_size);
    }
    if(strlen(fils[numfil].addrmult)>0){
        printf("Multicast for this fil is already initialised.\n");
        addresse_a_envoyer=(uint8_t *) fils[numfil].addrmult;
    }
    else{
        printf("INITIALISING MULTICAST ADDR.\n");
        // If fil does not have a multicast address then initialise it
        int multicast_sock;
        if((multicast_sock=socket(AF_INET6,SOCK_DGRAM,0))<0){
            perror("erreur socket");
            return;
        }

        // Initialisation de l'adresse d'abonnement
        struct sockaddr_in6 server_addr;
        memset(&server_addr,0,sizeof(server_addr));
        server_addr.sin6_family=AF_INET6;
        inet_pton(AF_INET6,address_str,&server_addr.sin6_addr);
        server_addr.sin6_port=htons(MULTICAST_PORT); //TODO pick port?

        // Initialisation de l'interface locale autorisant le multicast IPv6
        int ifindex=0; //if_nametoindex("eth0");
        if(setsockopt(multicast_sock,IPPROTO_IPV6,IPV6_MULTICAST_IF,&ifindex,sizeof(ifindex))){
            perror("erreur initialisation de l'interface locale");
            return;
        }

        /*
        // bind the socket
        int err=bind(multicast_sock,(struct sockaddr *) &server_addr,sizeof(server_addr));
        if(err==-1){
            perror("erreur de bind");
            exit(1);
        }
         */

        addresse_a_envoyer=malloc(sizeof(uint8_t)*16); // Allocate memory for the address
        memcpy(addresse_a_envoyer,server_addr.sin6_addr.s6_addr,sizeof(uint8_t)*16); // Copy the address bytes

        // Allocate memory for fils[numfil]->addrmult if it's not already allocated
        if(fils[numfil].addrmult==NULL){
            fils[numfil].addrmult=malloc(sizeof(uint8_t)*16);
        }

        // Copy the content of addresse_a_envoyer into fils[numfil]->addrmult
        memcpy(fils[numfil].addrmult,server_addr.sin6_addr.s6_addr,sizeof(uint8_t)*16);
    }

    // update fil subscription counter
    fils[numfil].subscribed+=1;

    //sending response to client
    server_subscription_message *msg=malloc(sizeof(server_subscription_message));
    msg->entete.val=create_entete(SUBSCRIBE,id)->val;
    msg->numfil=htons(numfil);
    msg->nb=htons(MULTICAST_PORT);
    msg->addrmult=malloc(sizeof(uint8_t)*16);
    memcpy(msg->addrmult,addresse_a_envoyer,sizeof(uint8_t)*16);

    print_bits(msg->entete.val);
    print_bits(msg->numfil);
    print_bits(msg->nb);
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
    printf("received_msg->numfil = %d\n", received_msg->numfil);
    printf("received_msg->entete.val = %d\n", received_msg->entete.val);
    printf("received_msg->nb = %d\n", received_msg->nb);
    printf("received_msg->data = %d%s\n", received_msg->data[0], received_msg->data + 1);
    // on fait les vérifications de pour ajouter un billet à un fil
    if (received_msg->numfil == 0) {
        add_new_fil(pseudo_from_id(get_id_entete(received_msg->entete.val)));
        received_msg->numfil = fils_size - 1;
        printf("Numfil trouvé vide, création d'un nouveau fil : %d\n", received_msg->numfil);
    } else if (received_msg->numfil >= fils_size) {
        printf("Demande d'écriture sur un fil inexistant\n");
        received_msg->numfil = 0;
    }

    printf("Création du socket UDP\n");
    // création socket UDP
    int sock_serv = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock_serv < 0) {
        perror("socket UDP");
    }

    printf("Configuration du délai d'attente\n");
    // on définit le délai d'attente de recvfrom à 30 secondes
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    if (setsockopt(sock_serv, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erreur lors de la définition du délai d'attente");
        return;
    }

    uint16_t port = allocate_port();
    printf("Configuration de l'adresse du serveur\n");
    struct sockaddr_in6 servadr;
    memset(&servadr, 0, sizeof(servadr));
    servadr.sin6_family = AF_INET6;
    servadr.sin6_addr = in6addr_any;
    servadr.sin6_port = htons(port);
    printf("port = %d\n", port);
    if (bind(sock_serv, (struct sockaddr *)&servadr, sizeof(servadr)) < 0) {
        perror("bind UDP:");
    }

    printf("Envoi du message avec le port au client\n");
    // envoie message avec le port au client
    send_message(5, ntohs(received_msg->entete.val) >> 5, port, received_msg->numfil, sock_client);

    printf("Écoute de la réponse du client\n");
    //  on écoute la réponse du client qu'on écrit dans un buffer
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    char recv_buffer[516];
    //int packet_num;
    size_t bytes_read;

    // chemin vers le stockage
    char file_directory[] = "fichiers_fil";

    FILE *file = NULL;

    do {

        printf("Attente de données\n");
        // on écoute
        bytes_read = recvfrom(sock_serv, recv_buffer, sizeof(char) * 512 + sizeof(u_int16_t) * 2, 0, (struct sockaddr *)&client_addr, &addr_len);

        // on vérifie que recvfrom n'a pas timeout
        if (bytes_read <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("recvfrom a expiré après 30 secondes\n");
                break;
            }else if(bytes_read == 0){
                printf("fin communication\n");
                break;
            } else {
                perror("Erreur lors de la réception des données");
                return;
            }
        }
        printf("Données reçues : %zu\n", bytes_read);

        // on deserialize
        client_message_udp *received_msg_udp = malloc(sizeof(client_message_udp));
        memcpy(&(received_msg_udp->entete.val), recv_buffer, sizeof(uint16_t));
        memcpy(&(received_msg_udp->numbloc), recv_buffer + sizeof(uint16_t), sizeof(uint16_t));
        received_msg_udp->data = malloc(sizeof(char) * bytes_read - sizeof(uint16_t) * 2);
        memcpy(received_msg_udp->data, recv_buffer + sizeof(uint16_t) * 2, sizeof(char) * bytes_read - sizeof(uint16_t) * 2);

        printf("received_msg_udp->entete.val = %d\n",received_msg_udp->entete.val);
        printf("received_msg_udp->numbloc = %d\n",received_msg_udp->numbloc);
        printf("received_msg_udp->data = %s\n",received_msg_udp->data);
        printf("ouverture du file\n");
        // on ouvre le FILE
        if (file == NULL) {
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%d_%s", file_directory, received_msg->numfil, received_msg->data + 1);
            printf("file_path = %s\n", file_path);
            file = fopen(file_path, "w");
            if (file == NULL) {
                perror("Erreur d'ouverture du fichier");
                return;
            }
        }

        printf("écriture dans le file\n");
        // on écrit dans le FILE
        fwrite(received_msg_udp->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);

        // todo gérer le cas ou les packets ne sont pas dans l'ordre
        //packet_num = received_msg_udp->numbloc;

        // si dernier paquet udp, on break
        if(bytes_read < sizeof(char) * 512 + sizeof(u_int16_t) * 2) break;
    } while (1);

    printf("fin while, release_port, fclose, close\n");
    if (file != NULL) {
        fclose(file);
    }
    release_port(servadr.sin6_port);
    close(sock_serv);

    // on ajoute le fichier au fil
    printf("on ajoute le fichier au fil\n");
    fil* current_fil = &fils[received_msg->numfil];

    message_node *new_node = malloc(sizeof(message_node));
    new_node->msg=malloc(sizeof(message));
    new_node->msg->datalen = received_msg->data[0];
    new_node->msg->id=get_id_entete(received_msg->entete.val);

    new_node->msg->data=malloc(sizeof(uint8_t) * new_node->msg->datalen);
    memcpy(new_node->msg->data,received_msg->data + 1,new_node->msg->datalen);

    new_node->next = current_fil->head;
    current_fil->head = new_node;

}



void *serve(void *arg){
    // on cherche codereq pour creer la structure correspondante et appeler la bonne fonction
    int sock_client=*((int *) arg);
    char buffer[sizeof(client_message) + sizeof(char)*BUFSIZ];
    ssize_t nb_octets=recv(sock_client,buffer,sizeof(buffer),0);

    if(nb_octets<0){
        perror("recv serve");
        exit(1);
        // gestion d'erreur
    }
    else if(nb_octets==0){
        printf("la connexion a été fermée\n");
        close(sock_client);
        return NULL;
        // la connexion a été fermée
    }
    else{
        // Vérification de la valeur de l'entête pour différencier les deux cas
        entete *header = (entete *) buffer;
        request_type codereq = (request_type) ntohs(header->val) & 0x1F;
        inscription *insc = NULL;
        printf("CODEREQ %d\n",codereq); // DEBUG

        client_message *received_msg=string_to_client_message(buffer);

        switch(codereq){
            case REGISTER:
                insc = (inscription *) buffer;
                inscription_client(insc->pseudo,sock_client);

            case POST_MESSAGE:
                if(received_msg->nb!=0)
                    send_error_message(sock_client);
                else
                    poster_billet(received_msg,sock_client);
                break;
            case LIST_MESSAGES:
                if(received_msg->datalen!=0)
                    send_error_message(sock_client);
                else
                    demander_liste_billets(received_msg,sock_client);
                break;
            case SUBSCRIBE:
                printf("User wants to join fil %d\n",ntohs(received_msg->numfil));
                add_subscription_to_fil(received_msg,sock_client);
                break;
            case UPLOAD_FILE:
                if(received_msg->nb!=0)
                    send_error_message(sock_client);
                else
                    add_file(received_msg,sock_client);
                break;
            default:
                perror("switch codereq");
                break;
        }
    }

    ssize_t ret=close(sock_client);
    if(ret<0){
        perror("close client's socket in thread");
        exit(1);
    }
    return NULL;
}


int main(){
    //*** creation de l'adresse du destinataire (serveur) ***
    struct sockaddr_in6 address_sock;
    memset(&address_sock,0,sizeof(address_sock));
    address_sock.sin6_family=AF_INET6;
    address_sock.sin6_port=htons(PORT);
    address_sock.sin6_addr=in6addr_any;

    //*** creation de la socket ***
    int sock=socket(PF_INET6,SOCK_STREAM,0);
    if(sock<0){
        perror("creation socket");
        exit(EXIT_FAILURE);
    }

    //*** desactiver l'option n'accepter que de l'IPv6 **
    int optval=0;
    int r_opt=setsockopt(sock,IPPROTO_IPV6,IPV6_V6ONLY,&optval,sizeof(optval));
    if(r_opt<0)
        perror("erreur connexion IPv4 impossible");

    //*** le numero de port peut etre utilise en parallele ***
    optval=1;
    r_opt=setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(r_opt<0)
        perror("erreur réutilisation de port impossible");

    //*** on lie la socket au port ***
    int r=bind(sock,(struct sockaddr *) &address_sock,sizeof(address_sock));
    if(r<0){
        perror("erreur bind");
        exit(EXIT_FAILURE);
    }

    //*** Le serveur est pret a ecouter les connexions sur le port ***
    r=listen(sock,0);
    if(r<0){
        perror("erreur listen");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("Erreur d'initialisation de sigaction");
        exit(EXIT_FAILURE);
    }
    //initialize the ports for UDP
    initialize_ports();

    // Initialise the fils
    fils = malloc(sizeof(fil));
    // The fil 0 has no originaire
    add_new_fil("");

    //Initialise the client list
    clients = malloc(sizeof(list_client));


    // Start the notification thread
    pthread_t notification_thread;
    pthread_create(&notification_thread, NULL, send_notifications, (void *)fils);

    while(running){
        struct sockaddr_in6 addrclient;
        socklen_t size=sizeof(addrclient);

        //*** on crée la varaiable sur le tas ***
        int *sock_client=malloc(sizeof(int));

        //*** le serveur accepte une connexion et initialise la socket de communication avec le client ***
        *sock_client=accept(sock,(struct sockaddr *) &addrclient,&size);

        if(sock_client>=0){
            pthread_t thread;
            //*** le serveur cree un thread et passe un pointeur sur socket client à la fonction serve ***
            if(pthread_create(&thread,NULL,serve,sock_client)==-1){
                perror("pthread_create");
                continue;
            }
            //*** affichage de l'adresse du client ***
            char nom_dst[INET6_ADDRSTRLEN];
            printf("Client connecte : %s %d\n",inet_ntop(AF_INET6,&addrclient.sin6_addr,nom_dst,sizeof(nom_dst)),
                   htons(addrclient.sin6_port));
        }
    }
    pthread_cancel(notification_thread);

    //*** fermeture socket serveur ***
    printf("Closing sock\n");
    close(sock);

    return 0;
}
