#include "../../include/client.h"


inscription *create_inscription(char pseudo[]){
    char new_pseudo[10];
    if(strlen(pseudo)!=10){
        strncpy(new_pseudo,pseudo,10);
        for(size_t i=strlen(new_pseudo); i<10; i++){
            new_pseudo[i]='#';
        }
    }
    inscription *inscription_message = malloc(sizeof(inscription));
    testMalloc(inscription_message);

    inscription_message->entete.val=create_entete(REGISTER,0)->val;
    strncpy(inscription_message->pseudo,new_pseudo,10);
    return inscription_message;
}

void send_message(res_inscription *i,char *data,int nbfil){
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);

    msg->entete.val = create_entete(POST_MESSAGE,i->id)->val;
    msg->numfil = htons(nbfil);
    uint8_t datalen = strlen(data)+1;
    msg->data = malloc(sizeof(uint8_t)*((datalen)+1));
    testMalloc(data);
    msg->datalen = datalen;
    memcpy(msg->data,data,sizeof(char)*(datalen));

    // Serialize the client_message structure and send to clientfd
    char *serialized_msg = client_message_to_string(msg);
    size_t len = sizeof(uint16_t) * 3 + sizeof(char) * (msg->datalen + 1);
    ssize_t ecrit = send(clientfd, serialized_msg, len, 0);

    if(ecrit<=0){
        fprintf(stderr, "Error writing: %s\n", strerror(errno));
        exit(3);
    }

    //*** reception d'un message ***

    char *buffer = malloc(sizeof(uint16_t)*3);
    testMalloc(buffer);

    memset(buffer,0,sizeof(uint16_t)*3);
    ssize_t recu = recv(clientfd,buffer,sizeof(uint16_t)*3,0);

    printf("retour du serveur reçu\n");
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }

    server_message *server_msg = string_to_server_message(buffer);
    request_type codereq = (request_type) ntohs(server_msg->entete.val) & 0x1F;
    if(codereq == NONEXISTENT_FIL){
      printf("Error: the fil you tried to post in doesn't exist.\n");
      return;
    }
    printf("Message écrit sur le fil %d\n", codereq);
}

char* pseudo_nohashtags(uint8_t* pseudo){
    int len = 10;
    while (len > 0 && pseudo[len-1] == '#')
        len--;

    char* str=malloc(sizeof(char)*11);
    testMalloc(str);
    memcpy(str, pseudo, len);
    str[len] = '\0';

    return str;
}

void print_n_tickets(char *server_msg,uint16_t numfil){
    server_message* received_msg = string_to_server_message(server_msg);

    request_type codereq = (request_type) ntohs(received_msg->entete.val) & 0x1F;
    if(codereq == NONEXISTENT_FIL){
      printf("Error: the fil you tried to list doesn't exist.\n");
      return;
    }


    printf("ID local: %d\n",user_id);
    printf("ID reçu: %d\n",get_id_entete(received_msg->entete.val));

    uint16_t nb_fil_serv=ntohs(received_msg->numfil);
    uint16_t nb_serv=ntohs(received_msg->nb);
    size_t count=sizeof(uint16_t)*3;

    if(numfil==0){
        printf("Nombre de fils à afficher: %d\n",nb_fil_serv);
        printf("Total de messages à afficher: %d\n",nb_serv);
    }
    else{
        printf("Fil affiché: %d\n",nb_fil_serv);
        printf("Nombre de messages à afficher: %d\n",nb_serv);
    }

    for(int i=0; i<nb_serv; i++){
        server_billet * received_billet=string_to_server_billet(server_msg+count);

        uint16_t fil=ntohs(received_billet->numfil);
        printf("\nFil %d\n",fil);
        // We don't show the name of the maker of the thread 0 since the server is creating it
        if(fil!=0){
            char* originaire=pseudo_nohashtags(received_billet->origine);
            printf("Originaire du fil: %s\n",originaire);
        }
        char* pseudo=pseudo_nohashtags(received_billet->pseudo);
        printf("\n\033[0;31m<%s>\033[0m ",pseudo);
        printf("%s\n",received_billet->data);

        count+=sizeof(uint16_t)+sizeof(uint8_t)*10+sizeof(uint8_t)*10+sizeof(uint8_t)*(received_billet->datalen+1);
    }

    printf("\n");
}

void request_n_tickets(res_inscription *i,uint16_t numfil,uint16_t n){
    client_message *msg=malloc(sizeof(client_message));
    testMalloc(msg);

    msg->entete.val=create_entete(LIST_MESSAGES,i->id)->val;
    msg->numfil=htons(numfil);
    msg->nb=htons(n);
    msg->datalen=0;
    msg->data=malloc(sizeof(uint8_t)*2);
    testMalloc(msg->data);

    printf("\nEntête envoyée au serveur:\n");
    print_bits(ntohs(msg->entete.val));

    ssize_t ecrit=send(clientfd,msg,sizeof(msg->entete)+sizeof(uint16_t)*2+sizeof(uint8_t)*2,0);
    if(ecrit<=0){
        perror("Erreur ecriture");
        exit(3);
    }
    printf("Message envoyé au serveur.\n");

    ssize_t buffer_size = BUFSIZ;
    char *buffer = malloc(sizeof(char)*buffer_size);
    testMalloc(buffer);

    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    while ((bytes_received = recv(clientfd, buffer + total_bytes_received, BUFSIZ, 0)) > 0) {
        printf("Octets reçus du serveur: %zu\n",bytes_received);
        total_bytes_received += bytes_received;

        // Check if the remaining buffer space is less than BUFSIZ
        if (buffer_size - total_bytes_received < BUFSIZ) {
            // Increase the buffer size by BUFSIZ
            buffer_size += BUFSIZ;

            // Reallocate the buffer to the new size
            char *new_buffer = realloc(buffer, buffer_size);
            testMalloc(new_buffer);
            buffer = new_buffer;
        }
    }

    if (bytes_received < 0) {
        perror("recv");
        free(buffer);
        return;
    }
    print_n_tickets(buffer,numfil);
}

res_inscription* send_inscription(inscription *i){
    ssize_t ecrit=send(clientfd,i,12,0);
    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }
    print_bits(i->entete.val);
    printf("demande d'inscription envoyée\n");

    char *buffer = malloc(sizeof(uint16_t)*3);
    testMalloc(buffer);
    memset(buffer,0,sizeof(uint16_t)*3);

    //*** reception d'un message ***
    ssize_t recu = recv(clientfd,buffer,sizeof(uint16_t)*3,0);
    printf("retour du serveur reçu\n");
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }
    server_message *server_msg = string_to_server_message(buffer);

    request_type codereq = (request_type) ntohs(server_msg->entete.val) & 0x1F;
    if(codereq == ERROR){
      printf("Error during registration. Maximum number of clients reached.\n");
      return NULL;
    }

    print_bits(ntohs(server_msg->entete.val));
    print_bits(ntohs(server_msg->numfil));
    print_bits(ntohs(server_msg->nb));


    res_inscription *res = malloc(sizeof(res_inscription));
    testMalloc(res);
    res->id=get_id_entete(server_msg->entete.val);

    return res;
}


void *listen_multicast_messages(void *arg) {
    // Extract the multicast address from the argument
    uint8_t *multicast_address = (uint8_t *)arg;

    // Create a socket for listening to multicast messages
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return NULL;
    }

    // Join the multicast group
    struct ipv6_mreq mreq;
    memcpy(&mreq.ipv6mr_multiaddr, multicast_address, sizeof(struct in6_addr));
    mreq.ipv6mr_interface = 0; // Let the system choose the interface

    if(setsockopt(sockfd,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0){
        perror("setsockopt(IPV6_ADD_MEMBERSHIP)");
        close(sockfd);
        return NULL;
    }

    int enable=1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable))<0){
        perror("setsockopt(SO_REUSEADDR)");
        close(sockfd);
        return NULL;
    }

    // Bind the socket to the multicast port
    struct sockaddr_in6 local_addr;
    memset(&local_addr,0,sizeof(local_addr));
    local_addr.sin6_family=AF_INET6;
    local_addr.sin6_addr=in6addr_any;
    local_addr.sin6_port=htons(MULTICAST_PORT);

    if(bind(sockfd,(struct sockaddr *) &local_addr,sizeof(local_addr))<0){
        perror("bind");
        close(sockfd);
        return NULL;
    }

    // Receive and process messages
    while(1){
        char buffer[262];
        struct sockaddr_in6 src_addr;
        socklen_t addrlen=sizeof(src_addr);

        ssize_t nbytes=recvfrom(sockfd,buffer,sizeof(buffer),0,
                                (struct sockaddr *) &src_addr,&addrlen);
        if(nbytes<0){
            perror("recvfrom");
            break;
        }

        // deserializer le message recu
        notification *notification=string_to_notification(buffer);

        // Process the received message
        printf("New post in the fil %d!\n",ntohs(notification->numfil));
        char *pseudo=pseudo_nohashtags(notification->pseudo);
        printf("\033[0;31m<%s>\033[0m ",pseudo);
        printf("%s",notification->data);
        if(strlen((char*) notification->data) >= 20) printf("...");
        printf("\n");
    }
    close(sockfd);
    return NULL;
}


void subscribe_to_fil(uint16_t fil_number) {
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);
    msg->entete.val=create_entete(SUBSCRIBE,user_id)->val;
    msg->numfil = htons(fil_number);

    ssize_t ecrit=send(clientfd,msg,sizeof(uint16_t)*3+sizeof(uint8_t)*1,0);
    if(ecrit<=0){
        perror("Erreur ecriture");
        exit(3);
    }

    // receive response from server with the multicast address
    char *server_msg = malloc(sizeof(char) * 22);
    testMalloc(server_msg);
    memset(server_msg,0,sizeof(char) * 22);

    ssize_t recu=recv(clientfd,server_msg,sizeof(char) * 22 ,0);
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }

    server_subscription_message *received_msg = string_to_server_subscription_message(server_msg);
    request_type codereq = (request_type) ntohs(received_msg->entete.val) & 0x1F;
    if(codereq == NONEXISTENT_FIL){
      printf("Error: the fil you tried to subscribe to doesn't exist.\n");
      return;
    }

    //  set up a separate thread to listen for messages on the multicast address
    pthread_t notification_thread;
    int rc = pthread_create(&notification_thread, NULL, listen_multicast_messages,
                            (void *)received_msg->addrmult);
    if (rc != 0) {
        perror("pthread_create");
        exit(1);
    }

}

void add_file(int nbfil) {
    printf("Création du message du client au serveur\n");
    // on crée le message du client au serveur
    client_message *msg = malloc(sizeof(client_message));

    msg->entete.val = create_entete(UPLOAD_FILE, user_id)->val;
    msg->nb = htons(0);
    msg->numfil = htons(nbfil);

    printf("Ouverture et vérification du fichier\n");
    // on ouvre le fichier pour vérifier qu'il existe
    FILE *file = NULL;
    long size_max = 1L << 25; // 2^25 octets
    while (1) {
        printf("Please enter a file emplacement: ");
        char *file_emplacement = malloc(sizeof(char) * 512);
        scanf("%s", file_emplacement);
        file = fopen(file_emplacement, "rb");
        if (file == NULL) {
            printf("The file emplacement is invalid.\n");
        } else {
            long size = size_file(file);
            if (size > size_max) {
                printf("The file is too large.\n");
                fclose(file);
            }
            break;
        }
    }

    printf("Récupération du nom du fichier\n");
    // on récupère le nom du fichier
    printf("Please enter the name of the file: ");
    char *filename = malloc(sizeof(char) * 512);
    scanf("%s", filename);

    uint8_t datalen = strlen(filename) + 1;
    msg->data = malloc(sizeof(char) * (datalen + 1));
    *msg->data = datalen;
    memcpy(msg->data + 1, filename, sizeof(char) * (datalen));
    printf("msg->entete.val = %d\n", msg->entete.val);
    printf("datalen = %d\n", ntohs(datalen));
    printf("msg->data = %d%s\n", msg->datalen, msg->data + 1);
    //msg->data + 1 = "test";
    //printf("msg->data = %d%s\n", msg->data[0], msg->data + 1);


    char *serialized_msg = client_message_to_string(msg);
    printf("serialization validée \n");



    printf("Envoi du message au serveur\n");
    ssize_t ecrit = send(clientfd, serialized_msg, sizeof(uint16_t) * 3 + sizeof(char) * (msg->datalen + 1), 0);

    if (ecrit <= 0) {
        perror("Erreur ecriture");
        exit(3);
    }

    printf("Réception du message du serveur\n");
    // reception du message serveur
    uint16_t server_msg[3];
    ssize_t recu = recv(clientfd, server_msg, 3 * sizeof(uint16_t), 0);

    printf("retour du serveur reçu\n");
    if (recu < 0) {
        perror("erreur lecture");
        exit(4);
    }
    if (recu == 0) {
        printf("serveur off\n");
        exit(0);
    }

    for(int i = 0; i < 3; i++){
        server_msg[i] = ntohs(server_msg[i]);
    }
    printf(" CODEREQ + ID = %hu\n", server_msg[0]);
    printf("NUMFIL = %hu\n", server_msg[1]);
    printf("NB (port) = %hu\n", server_msg[2]);




    close(clientfd);

    printf("Création et configuration du socket UDP\n");
    int sock_udp;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Création du socket
    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("Erreur de création du socket");
        return;
    }

    // Configuration de l'adresse du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_msg[2]);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Erreur lors de la conversion de l'adresse IP");
        close(sock_udp);
        return;
    }
    printf("Envoi du fichier au serveur via UDP\n");
// envoie au serveur en udp
    char buffer[512];
    int packet_num = 0;
    size_t bytes_read;

    client_message_udp * msg_udp = malloc(sizeof(client_message_udp));



    while ((bytes_read = fread(buffer, 1, 512, file)) > 0) {
        printf("Préparation du message UDP\n");
        msg_udp->entete = msg->entete;
        msg_udp->numbloc = packet_num;
        msg_udp->data = malloc(sizeof(char) * bytes_read);
        memcpy(msg_udp->data, buffer, sizeof(char) * bytes_read);
        printf("msg_udp->entete = %d\n",msg_udp->entete.val);
        printf("msg_udp->numbloc = %d\n",msg_udp->numbloc);
        printf("msg_udp->data = %s\n",msg_udp->data);


        // on serialize
        ssize_t serialize_buf_size = sizeof(char) * bytes_read + sizeof(uint16_t) * 2;
        char *serialize_buffer = malloc(serialize_buf_size);

        memcpy(serialize_buffer, &(msg_udp->entete.val), sizeof(uint16_t));
        memcpy(serialize_buffer + sizeof(uint16_t), &(msg_udp->numbloc), sizeof(uint16_t));
        memcpy(serialize_buffer + sizeof(uint16_t) * 2, msg_udp->data, bytes_read);

        printf("Envoi du message UDP\n");
        ssize_t bytes_sent = sendto(sock_udp, serialize_buffer, serialize_buf_size, 0,
                                    (struct sockaddr *) &server_addr, addr_len);

        packet_num += 1;
        free(serialize_buffer);

        // todo erreur envoie données
        if (bytes_sent < 0) {
            free(msg_udp);
            perror("Erreur lors de l'envoi des données");
            fclose(file);
            return;
        }
    }

    printf("Nettoyage et fermeture du fichier\n");
    free(msg_udp);
    fclose(file);
    close(sock_udp);
}



void client(){
    int fdsock=socket(PF_INET,SOCK_STREAM,0);
    if(fdsock==-1){
        perror("creation socket");
        exit(1);
    }

    //*** creation de l'adresse du destinataire (serveur) ***
    struct sockaddr_in address_sock;
    memset(&address_sock,0,sizeof(address_sock));
    address_sock.sin_family=AF_INET;
    address_sock.sin_port=htons(7777);
    inet_pton(AF_INET,"localhost",&address_sock.sin_addr);

    //*** demande de connexion au serveur ***
    int r=connect(fdsock,(struct sockaddr *) &address_sock,sizeof(address_sock));
    if(r==-1){
        perror("echec de la connexion");
        exit(2);
    }

    clientfd=fdsock;
}



void print_ascii(){
    printf("  ------------------------------------------------------------  \n"
           " |                                                            |\n"
           " |    __  __                        _                         |\n"
           " |   |  \\/  |                      | |                        |\n"
           " |   | \\  / | ___  __ _  __ _ _ __ | |__   ___  _ __   ___    |\n"
           " |   | |\\/| |/ _ \\/ _` |/ _` | '_ \\| '_ \\ / _ \\| '_ \\ / _ \\   |\n"
           " |   | |  | |  __/ (_| | (_| | |_) | | | | (_) | | | |  __/   |\n"
           " |   |_|  |_|\\___|\\__, |\\__,_| .__/|_| |_|\\___/|_| |_|\\___|   |\n"
           " |                 __/ |     | |                              |\n"
           " |                |___/      |_|                              |\n"
           " |                                                            |\n"
           " |                                                            |\n"
           " |               1 - Inscription                              |\n"
           " |               2 - Poster un billet                         |\n"
           " |               3 - Liste des n dernier billets              |\n"
           " |               4 - S'abonner à un fil                       |\n"
           " |               5 - Ajouter un fichier                       |\n"
           " |               6 - Télécharger un fichier                   |\n"
           " |                                                            |\n"
           "  ------------------------------------------------------------\n"
           "                                                  \n"
           "                                                  \n");
}

res_inscription *inscription_client(char pseudo[10]){
    inscription *i=create_inscription(pseudo);
    return send_inscription(i);
}


void run(){
    int choice;
    res_inscription *res_ins=NULL;

    while(1){
        print_ascii();

        while(1){
            printf("Entrez un chiffre entre 1 et 6:\n");
            if(scanf("%d",&choice)!=1){
                while(getchar()!='\n');
                continue;
            }
            if(choice>=1 && choice<=6) break;
        }

        char *response=malloc(1024*sizeof(char));
        testMalloc(response);
        size_t len = 0;

        char pseudo[10];
        int nbfil=0;
        int n=0;
        ssize_t read;

        if(res_ins==NULL && choice!=1){
            printf("\nYou need to be registered first (Press 1)\n\n");
            continue;
        }

        client();

        switch(choice){
            case 1:
                printf("Please enter your username: ");
                scanf("%s",pseudo);
                if(strlen(pseudo)>10){
                  printf("Error: The username must be 10 characters or less.\n");
                  break;
                }
                res_ins = inscription_client(pseudo);
                if(res_ins == NULL) exit(4);
                user_id = res_ins->id;
                printf("id: %d\n",res_ins->id);
                break;
            case 2:
                printf("Please enter the thread (0 for a new thread): ");
                scanf("%d",&nbfil);
                getchar();  // Consume the newline character

                printf("Message to post: ");
                read = getline(&response, &len, stdin);

                if (read != -1) {
                  // Remove the newline character from the end of the line
                  response[strcspn(response, "\n")] = '\0';
                  printf("NBFIL: %d\nInput: %s\n", nbfil, response);
                }
                else {
                  perror("getline() failed\n");
                  exit(1);
                }

                send_message(res_ins,response,nbfil);
                break;
            case 3:
                printf("Please enter the thread: ");
                scanf("%d",&nbfil);
                printf("Please enter the number of posts: ");
                scanf("%d",&n);

                request_n_tickets(res_ins,nbfil,n);
                break;
            case 4:
                printf("Please enter the thread: ");
                scanf("%d",&nbfil);

                subscribe_to_fil(nbfil);
                break;
            case 5:
                printf("Please enter the thread: ");
                scanf("%d",&nbfil);

                add_file(nbfil);
            case 6:
            default:
                exit(0);
        }

        printf("Connexion fermée avec le serveur\n");
        close(clientfd);
        free(response);
    }
}



int main(void){
    run();
    return 0;
}
