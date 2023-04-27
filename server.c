#include "server.h"


#define PORT 7777
#define SIZE_BUF 256

// même fonction que client.c (mettre dans utilities ou un .h/.c partagé ?)

void add_new_fil() {
    // If the list is full, reallocate memory to double the capacity
    if (fils_size == fils_capacity) {
        fils_capacity *= 2;
        fils = realloc(fils, fils_capacity * sizeof(fil));
        testMalloc(fils);
    }

    (*(fils+fils_size)).fil_number=fils_size;
    (*(fils+fils_size)).head=malloc(sizeof(message_node));
    (*(fils+fils_size)).last_multicasted_message=malloc(sizeof(message_node));
    (*(fils+fils_size)).subscribed=0;
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

    uint16_t entete=create_entete(4,0)->val;

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

void send_message(uint8_t codereq, uint16_t id, uint16_t nb, uint16_t numfil, int sock_client){
    // TODO SERIALIZE EN STRING
    server_message * msg = malloc(sizeof(server_message));

    msg->entete.val = create_entete(codereq,id)->val;
    msg->numfil = htons(numfil);
    msg->nb = htons(nb);

    int nboctet = send(sock_client, msg, sizeof(server_message), 0);
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

// Find the first empty fil
uint16_t get_first_empty_numfil(){
    return fils_size;
    /*
    for(uint16_t i=1; i<MAX_FIL; i++){
        if(fils[i]==NULL){
            return i;
        }
    }
    return 0;
     */
}

void add_message_to_fil(client_message *msg, uint16_t fil_number) {
    fil* current_fil = &fils[fil_number];

    message_node *new_node = malloc(sizeof(message_node));
    new_node->msg=malloc(sizeof(message));
    new_node->msg->datalen=*msg->data;
    new_node->msg->id=get_id_entete(msg->entete.val);

    new_node->msg->data=malloc(sizeof(uint8_t)*new_node->msg->datalen);
    memcpy(new_node->msg->data,msg->data+sizeof(uint8_t),new_node->msg->datalen);

    new_node->next = current_fil->head;
    current_fil->head = new_node;
}

char** retrieve_messages_from_fil(uint16_t fil_number) {
    fil* current_fil = &fils[fil_number];

    message_node *current = current_fil->head;
    char **messages = malloc(sizeof(char *) * 1024);
    int index = 0;
    while (current != NULL && current->msg != NULL) {
        messages[index] = strdup((char *)(current->msg->data)); // Assuming the data starts at msg.data[1]
        /*
        printf("Message: %s\n",current->msg->data);
        printf("Message de l'id: %d\n",current->msg->id);
        printf("Datalen : %d\n",current->msg->datalen);
        */
        index++;
        current = current->next;
    }
    messages[index] = NULL; // Add NULL at the end of the messages array
    return messages;
}

void poster_billet(client_message *msg,int sock_client){
    int id=ntohs(msg->entete.val)>>5;

    printf("Received message from user with id %d, to post to numfil %d\n",id,ntohs(msg->numfil));
    printf("The message is: %s\n",(char *) (msg->data+1));

    uint16_t numfil=ntohs(msg->numfil);
    printf("Numfil reçu: %d\n",numfil);

    if(numfil==0){
        add_new_fil();
        numfil=fils_size-1;
        printf("Numfil trouvé vide: %d\n",numfil);
        printf("Assigned new numfil = %d\n",numfil);
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

    //sending response to client
    send_message(2,id,0,numfil,sock_client);
}

void demander_liste_billets(client_message *msg, int sock_client){
    if(msg->numfil==0){
        // The size of the list of fils is the same as the id of the next empty fil
        // uint16_t numfil=get_first_empty_numfil();
    }
    else{
        /*
        if(fils[ntohs(msg->numfil)]==NULL){
            send_error_message(sock_client);
            return;
        }
         */

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

        if(updated_last_multicasted_message == NULL) updated_last_multicasted_message = current_message;
        current_message = current_message->next;
    }
    //printf("New last multicasted message: %c\n\n", *(fils[fil_index].last_multicasted_message->msg->data));
    if(updated_last_multicasted_message != NULL) fils[fil_index].last_multicasted_message = updated_last_multicasted_message;
    close(sockfd);
}

// Fonction qui vérifie un par un les fils, si il y a besoin d'envoyer
// des messages (par exemple il y a au moins un abonné et il y a au moins un message),
// on lance la fonction send_fil_notification
void *send_notifications(void *arg){
    while(1){
        for(int i=0; i<fils_size; i++){
            if(fils[i].head->msg!=NULL && fils[i].head != fils[i].last_multicasted_message && fils[i].subscribed>0){
                // Send notifications for the fil
                printf("Sending fil notif to fil %d\n",i);
                send_fil_notification(i);
            }
        }
        sleep(NOTIFICATION_INTERVAL);
    }

    return NULL;
}


void add_subscription_to_fil(client_message *received_msg,int sock_client){
    uint16_t numfil=ntohs(received_msg->numfil);
    int id=ntohs(received_msg->entete.val)>>5;

    //Prepare the multicast address: start at base and add numfil
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

    // check if fil already has a multicast address
    if(strlen(fils[numfil].addrmult)>0){
        printf("Multicast for this fil is already initialised.\n");
        addresse_a_envoyer=(uint8_t *) fils[numfil].addrmult;
    }
    else{
        printf("INITIALISING MULTICAST ADDR.\n");
        // if fil does not have a multicast address then initialise it
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

        // initialisation de l'interface locale autorisant le multicast IPv6
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
    msg->entete.val=create_entete(4,id)->val;
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
    int nboctet=send(sock_client,serialized_msg,serialized_msg_size,0);
    printf("DEBUG SENT %d OCTETS\n",nboctet);
    if(nboctet<=0)perror("send");
}

void *serve(void *arg){

// on cherche codereq pour creer la structure correspondante et appeler la bonne fonction
    int sock_client = *((int *) arg);
    char buffer[sizeof(client_message)];
    while(1) {
        int nb_octets = recv(sock_client, buffer, sizeof(buffer), 0);

        if (nb_octets < 0) {
            perror("recv serve");
            exit(1);
            // gestion d'erreur
        } else if (nb_octets == 0) {
            printf("la connexion a été fermée\n");
            break;
            // la connexion a été fermée
        }
        else {
            // Vérification de la valeur de l'entête pour différencier les deux cas
            entete *header = (entete *) buffer;
            uint8_t codereq = ntohs(header->val) & 0x1F;
            printf("CODEREQ %d\n", codereq); // DEBUG
            if (codereq == 1) {
                // le message reçu est de type inscription_message
                inscription *insc = (inscription *) buffer;

                inscription_client(insc->pseudo, sock_client);
            } else {

                client_message *received_msg = string_to_client_message(buffer);

                switch (codereq) {
                    case 2:
                        if(received_msg->nb!=0){
                            send_error_message(sock_client);
                        }
                        else{
                            poster_billet(received_msg, sock_client);
                        }
                        break;
                    case 3:
                        if(*received_msg->data!=0){
                            send_error_message(sock_client);
                        }
                        else{
                            demander_liste_billets(received_msg,sock_client);
                        }
                        break;
                    case 4:
                        printf("User wants to join fil %d\n", ntohs(received_msg->numfil));
                        add_subscription_to_fil(received_msg, sock_client);
                        break;
                    case 5:
                        if(received_msg->nb!=0){
                            send_error_message(sock_client);
                        }
                        else{

                        }
                        break;
                    default:
                        perror("switch codereq");
                        break;
                }
            }
        }
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

    // Initialise the fils
    fils = malloc(sizeof(fil));
    add_new_fil();

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
            printf("client connecte : %s %d\n",inet_ntop(AF_INET6,&addrclient.sin6_addr,nom_dst,sizeof(nom_dst)),
                   htons(addrclient.sin6_port));
        }
    }
    pthread_cancel(notification_thread);

    //*** fermeture socket serveur ***
    printf("Closing sock\n");
    close(sock);

    return 0;
}
