#include "../../include/client.h"

void print_prompt(){
    printf("> ");
}

int handle_codereq_error(request_type codereq){
    switch(codereq){
        case NONEXISTENT_FIL:
            printf("Error: the fil you tried to post in doesn't exist.\n");
            return 0;
        case NONEXISTENT_ID:
            printf("Error: your ID doesn't exist on the server.\n");
            return 0;
        default:
            return 1;
    }
}

// Fonction pour initialiser la liste des ports disponibles
void initialize_ports() {
    available_ports = malloc(sizeof(int) * PORT_RANGE);
    testMalloc(available_ports);
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
    memcpy(msg->data,data,sizeof(uint8_t)*(datalen));

    // Serialize the client_message structure and send to clientfd
    char *serialized_msg = client_message_to_string(msg);
    // todo modifier datalen + 1 en datalen ? (puisque datalen = strlen(data) + 1)
    size_t len = CLIENT_MESSAGE_SIZE + sizeof(char) * (msg->datalen + 1);
    ssize_t ecrit = send(clientfd, serialized_msg, len, 0);

    if(ecrit<=0){
        fprintf(stderr, "Error writing: %s\n", strerror(errno));
        exit(3);
    }

    //*** reception d'un message ***

    char *buffer = malloc(SERVER_MESSAGE_SIZE);
    testMalloc(buffer);
    memset(buffer,0,SERVER_MESSAGE_SIZE);

    ssize_t read = recv_bytes(clientfd,buffer,SERVER_MESSAGE_SIZE);
    // ssize_t recu = recv(clientfd,buffer,SERVER_MESSAGE_SIZE,0);

    printf("retour du serveur reçu\n");
    if(read < 0){
        perror("erreur lecture");
        exit(4);
    }
    if(read == 0){
        printf("serveur off\n");
        exit(0);
    }

    server_message *server_msg = string_to_server_message(buffer);
    request_type codereq = get_codereq_entete(server_msg->entete.val);
    if(!handle_codereq_error(codereq)){
        return;
    }
    printf("Message écrit sur le fil %d\n", ntohs(server_msg->numfil));
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
    server_message *received_msg = string_to_server_message(server_msg);
    request_type codereq = get_codereq_entete(received_msg->entete.val);

    if(!handle_codereq_error(codereq)){
        return;
    }

    printf("ID local: %d\n",user_id);
    printf("ID reçu: %d\n",get_id_entete(received_msg->entete.val));

    uint16_t nb_fil_serv = ntohs(received_msg->numfil);
    uint16_t nb_serv = ntohs(received_msg->nb);
    size_t offset = SERVER_MESSAGE_SIZE;

    if(numfil == 0){
        printf("Nombre de fils à afficher: %d\n",nb_fil_serv);
        printf("Total de messages à afficher: %d\n",nb_serv);
    }
    else{
        printf("Fil affiché: %d\n",nb_fil_serv);
        printf("Nombre de messages à afficher: %d\n",nb_serv);
    }

    for(int i=0; i<nb_serv; i++){
        server_billet *received_billet = string_to_server_billet(server_msg+offset);
        uint16_t fil = ntohs(received_billet->numfil);
        printf("\nFil %d\n",fil);

        char* pseudo=pseudo_nohashtags(received_billet->pseudo);
        printf("\n\033[0;31m<%s>\033[0m ",pseudo);
        printf("%s\n",received_billet->data);

        // todo modifier datalen + 1 en datalen ? (puisque datalen = strlen(data) + 1)
        offset+=NUMFIL_SIZE+ORIGINE_SIZE+PSEUDO_SIZE+(sizeof(uint8_t)*(received_billet->datalen+1));
    }
    printf("\n");
}

void request_n_tickets(res_inscription *i,uint16_t numfil,uint16_t n){
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);

    msg->entete.val = create_entete(LIST_MESSAGES,i->id)->val;
    msg->numfil = htons(numfil);
    msg->nb = htons(n);
    msg->datalen = 0;

    printf("\nEntête envoyée au serveur:\n");
    print_bits(ntohs(msg->entete.val));

    ssize_t ecrit = send(clientfd,msg,CLIENT_MESSAGE_SIZE+DATALEN_SIZE,0);
    if(ecrit<=0){
        perror("Erreur ecriture");
        exit(3);
    }
    printf("Message envoyé au serveur.\n");

    ssize_t buffer_size = BUFSIZ;
    char *buffer = malloc(sizeof(char)*BUFSIZ);
    testMalloc(buffer);

    ssize_t ret = recv_unlimited_bytes(clientfd,buffer,buffer_size);
    if(ret < 0){
        free(buffer);
        perror("Receive unlimited bytes when requesting n tickets");
        exit(1);
    }

    print_n_tickets(buffer,numfil);
}

res_inscription* send_inscription(inscription *i){
    ssize_t ecrit = send(clientfd,i,REGISTER_SIZE,0);
    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }
    print_bits(i->entete.val);
    printf("demande d'inscription envoyée\n");

    char *buffer = malloc(SERVER_MESSAGE_SIZE);
    testMalloc(buffer);
    memset(buffer,0,SERVER_MESSAGE_SIZE);

    //*** reception d'un message ***
    // ssize_t recu = recv(clientfd,buffer,SERVER_MESSAGE_SIZE,0);
    ssize_t read = recv_bytes(clientfd,buffer,SERVER_MESSAGE_SIZE);
    printf("retour du serveur reçu\n");
    if(read<0){
        perror("Recv error in the send inscription function");
        exit(4);
    }
    if(read==0){
        printf("Server off");
        exit(0);
    }
    server_message *server_msg = string_to_server_message(buffer);

    request_type codereq = get_codereq_entete(server_msg->entete.val);
    if(codereq == ERROR){
        printf("Error during registration. Maximum number of clients reached.\n");
        return NULL;
    }

    // print_bits(ntohs(server_msg->entete.val));
    // print_bits(ntohs(server_msg->numfil));
    // print_bits(ntohs(server_msg->nb));

    res_inscription *res = malloc(sizeof(res_inscription));
    testMalloc(res);
    res->id=get_id_entete(server_msg->entete.val);

    return res;
}

void *listen_multicast_messages(void *arg) {
    // Extract the multicast address from the argument
    server_subscription_message *received_msg = (server_subscription_message *)arg;
    uint8_t *multicast_address = received_msg->addrmult;

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
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_addr = in6addr_any;
    local_addr.sin6_port = received_msg->nb;

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

        ssize_t nbytes = recvfrom(sockfd,buffer,sizeof(buffer),0,
                                (struct sockaddr *) &src_addr,&addrlen);
        if(nbytes<0){
            perror("recvfrom");
            break;
        }

        // deserializer le message recu
        notification *notification = string_to_notification(buffer);

        // Process the received message
        printf("\n\nNew post in the fil %d!\n",ntohs(notification->numfil));
        char *pseudo = pseudo_nohashtags(notification->pseudo);
        printf("\033[0;31m<%s>\033[0m ",pseudo);
        printf("%s",notification->data);
        if(strlen((char*) notification->data) >= 20) printf("...");
        printf("\n\n");
        print_prompt();
        fflush(stdout);
    }
    close(sockfd);
    return NULL;
}

void subscribe_to_fil(uint16_t fil_number) {
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);
    msg->entete.val = create_entete(SUBSCRIBE,user_id)->val;
    msg->numfil = htons(fil_number);

    ssize_t ecrit = send(clientfd,msg,CLIENT_MESSAGE_SIZE+DATALEN_SIZE,0);
    if(ecrit<=0){
        perror("Erreur ecriture");
        exit(3);
    }

    // receive response from server with the multicast address
    char *server_msg = malloc(SERVER_SUBSCRIPTION_SIZE);
    testMalloc(server_msg);
    memset(server_msg,0,SERVER_SUBSCRIPTION_SIZE);

    // ssize_t recu = recv(clientfd,server_msg,sizeof(char) * 22 ,0);
    ssize_t read = recv_bytes(clientfd,server_msg,SERVER_SUBSCRIPTION_SIZE);
    if(read < 0){
        perror("erreur lecture");
        exit(4);
    }
    if(read == 0){
        printf("serveur off\n");
        exit(0);
    }

    server_subscription_message *received_msg = string_to_server_subscription_message(server_msg);
    request_type codereq = get_codereq_entete(received_msg->entete.val);
    printf("Got codereq %d\n", codereq);
    if(!handle_codereq_error(codereq)){
        return;
    }

    //  set up a separate thread to listen for messages on the multicast address
    pthread_t notification_thread;
    int rc = pthread_create(&notification_thread, NULL, listen_multicast_messages,
                            (void *)received_msg);
    if (rc != 0) {
        perror("pthread_create");
        exit(1);
    }

}

void download_file(int nbfil){
    printf("Création du premier message du client au serveur\n");

    // on crée le message du client au serveur
    client_message *msg = malloc(sizeof(client_message));
    int port = allocate_port();
    msg->entete.val = create_entete(DOWNLOAD_FILE, user_id)->val;
    msg->nb = htons(port);
    msg->numfil = htons(nbfil);

    // on récupère le nom du fichier
    printf("Récupération du nom du fichier\n");
    char *filename = malloc(sizeof(char) * 512);
    testMalloc(filename);
    while(1){
        // on récupère le nom du fichier
        printf("Please enter the name of the file (<512 characters) : ");
        scanf("%s", filename);
        if(is_valid_filename(filename)) break;
        printf("File name invalid\n");
    }

    // on finit de remplir le message du client au serveur
    uint8_t datalen = strlen(filename) + 1; // + 1 pour le '\0'
    msg->datalen = datalen;
    msg->data = malloc(sizeof(char) * (datalen));
    testMalloc(msg->data);
    memcpy(msg->data, filename, sizeof(char) * (datalen));
    msg->data[datalen] = '\0';
    printf("\n\nmsg->entete.val = %d\n", msg->entete.val);
    printf("msg->datalen = %d\n", datalen);
    printf("msg->data = %s\n", msg->data);



    // on envoie le message au serveur
    char *serialized_msg = client_message_to_string(msg);
    printf("\nserialization validée \n");


    printf("\n\nEnvoi du premier message au serveur\n");
    ssize_t ecrit = send(clientfd, serialized_msg,
                         CLIENT_MESSAGE_SIZE + DATALEN_SIZE + sizeof(char) * (msg->datalen), 0);
    printf("données envoyées lors du premier message = %zu\n",ecrit);


    //free
    free(serialized_msg);
    free(msg->data);
    free(msg);

    if (ecrit <= 0) {
        perror("Erreur ecriture");
        goto error;
    }

    // reception du message serveur retour (en TCP)
    printf("\n\nRéception du message du serveur\n");
    uint16_t server_msg[3];
    ssize_t recu = recv_bytes(clientfd,(char*) server_msg, SERVER_MESSAGE_SIZE);

    if (recu == (size_t) -1) {
        perror("erreur lecture");
        goto error;
    }
    if (recu == 0) {
        printf("serveur off\n");
        goto error;
    }

    for(int i = 0; i < 3; i++){
        server_msg[i] = ntohs(server_msg[i]);
    }

    request_type codereq = get_codereq_entete(server_msg[0]);
    if(!handle_codereq_error(codereq)){
        goto error;
    }

    printf("message retour du serveur: \n");
    printf("entete = %hu\n", server_msg[0]);
    printf("nb fil = %hu\n", server_msg[1]);
    printf("port = %hu\n", server_msg[2]);
    printf("retour du serveur reçu\n");

    printf("\n\nappel boucle ecoute udp \n\n");
    // appelle a la boucle qui écoute le message en UDP
    // arguments -> dossier ou download, la socket udp, le port, le nom du fichier a download
    receive_file_udp("downloaded_files", port, nbfil, filename);

    // free et nettoyage
    free(filename);
    release_port(port);
    return;

    error:
    release_port(port);
    free(filename);

}

void add_file(int nbfil) {
    printf("Création du message du client au serveur\n");
    // on crée le message du client au serveur
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);

    msg->entete.val = create_entete(UPLOAD_FILE, user_id)->val;
    msg->nb = htons(0);
    msg->numfil = htons(nbfil);

    printf("Ouverture et vérification du fichier\n");
    // on ouvre le fichier pour vérifier qu'il existe
    FILE *file = NULL;
    long size_max = 1L << 25; // 2^25 octets
    while (1) {
        printf("Please enter a file path: ");
        char *file_path = malloc(sizeof(char) * 512);
        testMalloc(file_path);
        scanf("%s", file_path);
        file = fopen(file_path, "rb");
        free(file_path);
        if (file == NULL) {
            printf("The file path is invalid.\n");
        } else {
            long size = size_file(file);
            if (size > size_max) {
                printf("The file is too large.\n");
                fclose(file);
                return;
            }
            break;
        }
    }

    printf("Récupération du nom du fichier\n");
    char *filename = malloc(sizeof(char) * 512);
    testMalloc(filename);
    while(1){
        // on récupère le nom du fichier
        printf("Please enter the name of the file (<512 characters) : ");
        scanf("%s", filename);
        if(is_valid_filename(filename)) break;
        printf("File name invalid\n");
    }


    uint8_t datalen = strlen(filename) + 1; // + 1 pour le '\0'
    msg->datalen = datalen;
    msg->data = malloc(sizeof(char) * (datalen));
    testMalloc(msg->data);
    memcpy(msg->data, filename, sizeof(char) * (datalen));
    msg->data[datalen - 1] = '\0';
    printf("msg->entete.val = %d\n", msg->entete.val);
    printf("datalen = %d\n", datalen);
    printf("msg->data = %s\n", msg->data);

    char *serialized_msg = client_message_to_string(msg);
    printf("serialization validée \n");

    printf("Envoi du message au serveur\n");
    ssize_t ecrit = send(clientfd, serialized_msg, CLIENT_MESSAGE_SIZE + DATALEN_SIZE + sizeof(char) * (msg->datalen), 0);

    if (ecrit == (size_t) -1) {
        perror("Erreur ecriture");
        goto error;
    }
    if (ecrit == 0) {
        printf("serveur off\n");
        goto error;

    }

    printf("Réception du message du serveur\n");
    // reception du message serveur
    uint16_t server_msg[3];
    ssize_t recu = recv_bytes(clientfd,(char*)server_msg, SERVER_MESSAGE_SIZE);

    printf("retour du serveur reçu\n");
    if (recu == (size_t) -1) {
        perror("erreur lecture");
        goto error;
    }
    if (recu == 0) {
        printf("serveur off\n");
        goto error;
    }

    for(int i = 0; i < 3; i++){
        server_msg[i] = ntohs(server_msg[i]);
    }
    printf(" CODEREQ + ID = %hu\n", server_msg[0]);
    printf("NUMFIL = %hu\n", server_msg[1]);
    printf("NB (port) = %hu\n", server_msg[2]);

    close(clientfd);

    request_type codereq = get_codereq_entete(server_msg[0]);
    if(!handle_codereq_error(codereq)){
        goto error;
    }


    printf("\n\nappel a boucle envoie udp\n");
    // appel a boucle envoie udp avec en argument ->
    // le FILE, le port, le message du client (UNIQUEMENT pour l'entete)
    send_file_udp(file,server_msg[2], msg, server_addr);

    // nettoyage et free
    free(filename);
    free(serialized_msg);
    free(msg->data);
    free(msg);
    fclose(file);
    return;

    error:
    free(msg->data);
    free(msg);
    free(filename);
    free(serialized_msg);
    fclose(file);

}

void client(){
    int fdsock=socket(PF_INET6,SOCK_STREAM,0);
    if(fdsock==-1){
        perror("Socket creation");
        exit(1);
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

    //*** demande de connexion au serveur ***
    int r = connect(fdsock,(struct sockaddr *) &address_sock,sizeof(address_sock));
    if(r==-1){
        perror("Connection to server failed");
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
    res_inscription *res_ins = NULL;
    initialize_ports();

    while(1){
        print_ascii();

        while(1){
            printf("Entrez un chiffre entre 1 et 6:\n");
            print_prompt();
            if(scanf("%d",&choice)!=1){
                while(getchar()!='\n');
                continue;
            }
            if(choice>=1 && choice<=6) break;
        }

        char *response = malloc(1024*sizeof(char));
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
                printf("Please enter your username:\n");
                print_prompt();
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
                printf("Please enter the thread (0 for a new thread):\n");
                print_prompt();
                scanf("%d",&nbfil);
                getchar();  // Consume the newline character

                printf("Message to post:\n");
                print_prompt();
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
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);
                printf("Please enter the number of posts:\n");
                print_prompt();
                scanf("%d",&n);

                request_n_tickets(res_ins,nbfil,n);
                break;
            case 4:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);

                subscribe_to_fil(nbfil);
                break;
            case 5:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);

                add_file(nbfil);
                break;
            case 6:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);

                download_file(nbfil);
                break;
            default:
                exit(0);
        }

        printf("Connexion fermée avec le serveur\n");
        close(clientfd);
        free(response);
    }
}

int main(int argc, char** argv){
    if(argc == 1){
        server_addr = LOCAL_ADDR;
        server_port = PORT;
    }
    else if (argc != 3) {
        printf("Usage: %s <Server IP address> <Server port>\nOr: %s to launch locally on the port %d\n", argv[0], argv[0], PORT);
        exit(EXIT_FAILURE);
    }
    else{
        server_addr = argv[1];
        server_port = atoi(argv[2]);
        if(server_port < 1024){
          printf("Please enter a valid port in the range [1024-65535]\n");
          exit(EXIT_FAILURE);
        }
    }
    run();
    return 0;
}
