#include "../../include/client.h"

typedef struct uint_8 uint_8;

void print_prompt(){
    printf("> ");
}

int handle_codereq_error(request_type codereq){
    switch(codereq){
        case NONEXISTENT_THREAD:
            printf("Error: the thread doesn't exist.\n");
            return 0;
        case NONEXISTENT_ID:
            printf("Error: your ID doesn't exist on the server.\n");
            return 0;
        case ERROR:
            printf("Error: internal server error.\n");
            return 0;
        case NONEXISTENT_FILE:
            printf("Error: the file you tried to download does not exist on the server.\n");
            return 0;
        default:
            return 1;
    }
}

// Function to initialize th elist of accesible ports
void initialize_ports() {
    available_ports = malloc(sizeof(int) * PORT_RANGE);
    testMalloc(available_ports);
    for (uint16_t i = 0; i < PORT_RANGE; i++) {
        available_ports[i] = MIN_PORT + i;
    }
}

// Function to allocate a port
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

// Function to free a port
void release_port(int port) {
    if (port >= MIN_PORT && port <= MAX_PORT) {
        pthread_mutex_lock(&port_mutex);
        available_ports[port - MIN_PORT] = port;
        pthread_mutex_unlock(&port_mutex);
    }
}

inscription *create_inscription(char pseudo[]){
    char new_pseudo[10];
    strncpy(new_pseudo,pseudo,10);
    if(strlen(pseudo)!=10){
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
    msg->nb = 0;
    uint8_t datalen = strlen(data)+1;
    msg->datalen = datalen;
    msg->data = malloc(sizeof(uint8_t)*((datalen)));
    testMalloc(data);
    memcpy(msg->data,data,sizeof(uint8_t)*(datalen));

    // Serialize the client_message structure and send to clientfd
    char *serialized_msg = client_message_to_string(msg);

    size_t len = CLIENT_MESSAGE_SIZE + DATALEN_SIZE + sizeof(char) * (msg->datalen);
    ssize_t ecrit = send(clientfd, serialized_msg, len, 0);

    if(ecrit<=0){
        fprintf(stderr, "Error writing: %s\n", strerror(errno));
        exit(3);
    }

    //*** message reception ***
    char *buffer = malloc(SERVER_MESSAGE_SIZE);
    testMalloc(buffer);
    memset(buffer,0,SERVER_MESSAGE_SIZE);
    ssize_t read = recv_bytes(clientfd,buffer,SERVER_MESSAGE_SIZE);

    printf("retour du serveur reçu\n");
    if(read < 0){
        perror("Error recv");
        exit(4);
    }
    if(read == 0){
        printf("Server closed\n");
        exit(0);
    }

    server_message *server_msg = string_to_server_message(buffer);
    request_type codereq = get_codereq_entete(server_msg->entete.val);
    if(!handle_codereq_error(codereq)){
        return;
    }
    printf("Message has been posted on the thread %d\n", ntohs(server_msg->numfil));
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

    uint16_t nb_fil_serv = ntohs(received_msg->numfil);
    uint16_t nb_serv = ntohs(received_msg->nb);
    size_t offset = SERVER_MESSAGE_SIZE;

    if(numfil == 0){
        printf("Number of threads to print: %d\n",nb_fil_serv);
        printf("Total number of messages to print: %d\n",nb_serv);
    }
    else{
        printf("Printed thread: %d\n",nb_fil_serv);
        printf("Number of messages to print: %d\n",nb_serv);
    }

    if(nb_serv == 0){
        printf("\nNo messages.\n");
    }
    else{
        for(int i=0; i<nb_serv; i++){
            server_billet *received_billet = string_to_server_billet(server_msg+offset);
            uint16_t fil = ntohs(received_billet->numfil);
            printf("\nFil %d\n",fil);
            if(fil!=0){
                char* originaire=pseudo_nohashtags(received_billet->origine);
                printf("Originaire du fil: %s\n",originaire);
            }
            char* pseudo=pseudo_nohashtags(received_billet->pseudo);
            printf("\n\033[0;31m<%s>\033[0m ",pseudo);
            printf("%s\n",received_billet->data);

            offset += NUMFIL_SIZE + ORIGINE_SIZE + PSEUDO_SIZE + DATALEN_SIZE + (sizeof(uint8_t)*(received_billet->datalen));
        }
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
        perror("Error send");
        exit(3);
    }
    printf("Message envoyé au serveur.\n");

    ssize_t buffer_size = BUFSIZ;
    char *buffer = malloc(sizeof(char)*BUFSIZ);
    testMalloc(buffer);

    ssize_t ret = recv_unlimited_bytes(clientfd,buffer,buffer_size);
    if(ret < 0){
        free(buffer);
        perror("Received unlimited bytes when requesting n tickets");
        exit(1);
    }

    print_n_tickets(buffer,numfil);
}

res_inscription* send_inscription(inscription *i){
    for(int j=0; j<10; ++j){
        printf("%c\n",i->pseudo[j]);
    }
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

    //*** message reception ***
    ssize_t read = recv_bytes(clientfd,buffer,SERVER_MESSAGE_SIZE);
    printf("retour du serveur reçu\n");
    if(read<0){
        perror("Recv error in the send inscription function");
        exit(4);
    }
    if(read==0){
        printf("Server closed");
        exit(0);
    }
    server_message *server_msg = string_to_server_message(buffer);

    request_type codereq = get_codereq_entete(server_msg->entete.val);
    if(codereq == ERROR){
        printf("Error during registration. Maximum number of clients reached.\n");
        return NULL;
    }

    res_inscription *res = malloc(sizeof(res_inscription));
    testMalloc(res);
    res->id = get_id_entete(server_msg->entete.val);

    return res;
}

/**
 * This function listens to multicast messages from a subscribed fil on a separate thread.
 * It opens a socket, sets up multicast listening and handles the incoming messages.
 *
 * @param arg A pointer to a server_subscription_message structure which includes the multicast address and the fil number.
 */
void *listen_multicast_messages(void *arg) {
    server_subscription_message *received_msg = (server_subscription_message *)arg;
    uint8_t *multicast_address = received_msg->addrmult;

    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return NULL;
    }

    struct ipv6_mreq mreq;
    inet_pton (AF_INET6, (char *) multicast_address, &mreq.ipv6mr_multiaddr.s6_addr);
    mreq.ipv6mr_interface = 0; // Let the OS choose the interface
    if(setsockopt(sockfd,IPPROTO_IPV6,IPV6_JOIN_GROUP,&mreq,sizeof(mreq))<0){
        perror("Error setsockopt(IPV6_JOIN_GROUP)");
        close(sockfd);
        return NULL;
    }

    int enable=1;
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable))<0){
        perror("Error setsockopt(SO_REUSEADDR)");
        close(sockfd);
        return NULL;
    }

    struct sockaddr_in6 local_addr;
    memset(&local_addr,0,sizeof(local_addr));
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_addr = in6addr_any;
    local_addr.sin6_port = received_msg->nb;

    if(bind(sockfd,(struct sockaddr *) &local_addr,sizeof(local_addr))<0){
        perror("Error bind");
        close(sockfd);
        return NULL;
    }

    // Loop which receives notifications
    while(1){
        char buffer[BUFSIZ];
        struct sockaddr_in6 src_addr;
        socklen_t addrlen=sizeof(src_addr);

        ssize_t nbytes = recvfrom(sockfd,buffer,sizeof(buffer),0,(struct sockaddr *) &src_addr,&addrlen);
        if(nbytes < 0){
            perror("Error recvfrom");
            break;
        }

        // Deserialize the received message
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

/**
 * This function sends a subscribe codereq 4 to the server to subscribe to a particular thread.
 * If the subscription is successful, a new thread is created to listen to multicast messages from the subscribed fil.
 *
 * @param thread_number The number of the thread that the client wants to subscribe to.
 */
void subscribe_to_thread(uint16_t thread_number) {
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);
    msg->entete.val = create_entete(SUBSCRIBE,user_id)->val;
    msg->numfil = htons(thread_number);

    ssize_t ecrit = send(clientfd,msg,CLIENT_MESSAGE_SIZE+DATALEN_SIZE,0);
    if(ecrit<=0){
        perror("Error send");
        exit(3);
    }

    // Receive response from server with the multicast address
    char *server_msg = malloc(SERVER_SUBSCRIPTION_SIZE);
    testMalloc(server_msg);
    memset(server_msg,0,SERVER_SUBSCRIPTION_SIZE);

    ssize_t read = recv_bytes(clientfd,server_msg,SERVER_SUBSCRIPTION_SIZE);
    if(read < 0){
        perror("Error recv");
        exit(4);
    }
    if(read == 0){
        printf("Server closed\n");
        exit(0);
    }

    // Handle the received message
    server_subscription_message *received_msg = string_to_server_subscription_message(server_msg);
    request_type codereq = get_codereq_entete(received_msg->entete.val);
    if(!handle_codereq_error(codereq)){
        return;
    }

    // Set up a separate thread to listen for messages on the multicast address
    pthread_t notification_thread;
    int rc = pthread_create(&notification_thread, NULL, listen_multicast_messages, (void *)received_msg);
    if (rc != 0) {
        perror("pthread_create");
        exit(1);
    }
}

/**
 * The client download one of the files on a thread.
 * @param nbfil thread where the message is
 */
void download_file(int nbfil){
    printf("Creation of the first message from client to server\n");
    // create the message from the client to the server
    client_message *msg = malloc(sizeof(client_message));
    int port = allocate_port();
    msg->entete.val = create_entete(DOWNLOAD_FILE, user_id)->val;
    msg->nb = htons(port);
    msg->numfil = htons(nbfil);

    // we get the name of the file
    char *filename = malloc(sizeof(char) * 512);
    testMalloc(filename);
    while(1){
        printf("Please enter the name of the file (<512 characters) : ");
        scanf("%s", filename);
        if(is_valid_filename(filename)) break;
        printf("File name invalid\n");
    }

    // we build and send the message to the server
    uint8_t datalen = strlen(filename) + 1; // + 1 pour le '\0'
    msg->datalen = datalen;
    msg->data = malloc(sizeof(char) * (datalen));
    testMalloc(msg->data);
    memcpy(msg->data, filename, sizeof(char) * (datalen));
    msg->data[datalen - 1] = '\0';

    char *serialized_msg = client_message_to_string(msg);

    printf("sending the client message to server\n");
    ssize_t ecrit = send(clientfd, serialized_msg,
                         CLIENT_MESSAGE_SIZE + DATALEN_SIZE + sizeof(char) * (msg->datalen), 0);

    //free
    free(serialized_msg);
    free(msg->data);
    free(msg);

    if (ecrit <= 0) {
        perror("writing error");
        goto error;
    }

    // reception of the return server message (in TCP)
    printf("\nReceive server message\n");
    uint16_t server_msg[3];
    ssize_t recu = recv_bytes(clientfd,(char*) server_msg, SERVER_MESSAGE_SIZE);

    if (recu == (size_t) -1) {
        perror("reading error");
        goto error;
    }
    if (recu == 0) {
        printf("server off\n");
        goto error;
    }

    // codereq check
    request_type codereq = get_codereq_entete(server_msg[0]);
    if(!handle_codereq_error(codereq)){
        goto error;
    }

    // call the function that listens to the message in UDP
    receive_file_udp("downloaded_files", port, nbfil, filename);

    free(filename);
    release_port(port);
    return;

    error:
    release_port(port);
    free(filename);

}

/**
 * The client post one of his files on a thread.
 * @param nbfil thread where the message is added
 */
void add_file(int nbfil) {
    // we create the message from the client to the server
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);

    msg->entete.val = create_entete(UPLOAD_FILE, user_id)->val;
    msg->nb = htons(0);
    msg->numfil = htons(nbfil);

    printf("Open and check the file\n");
    // we open the file to check that it exists
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
        }
        else {
            long size = size_file(file);
            if (size > size_max) {
                printf("The file is too large.\n");
                fclose(file);
                return;
            }
            break;
        }
    }
    char *filename = malloc(sizeof(char) * 512);
    testMalloc(filename);
    while(1){
        printf("Please enter the name of the file (<512 characters) : ");
        scanf("%s", filename);
        if(is_valid_filename(filename)) break;
        printf("File name invalid\n");
    }

    // we build and send the message to the server
    uint8_t datalen = strlen(filename) + 1; // + 1 pour le '\0'
    msg->datalen = datalen;
    msg->data = malloc(sizeof(char) * (datalen));
    testMalloc(msg->data);
    memcpy(msg->data, filename, sizeof(char) * (datalen));
    msg->data[datalen - 1] = '\0';

    char *serialized_msg = client_message_to_string(msg);

    printf("Send message to server\n");
    ssize_t ecrit = send(clientfd, serialized_msg, CLIENT_MESSAGE_SIZE + DATALEN_SIZE + sizeof(char) * (msg->datalen), 0);

    if (ecrit == (size_t) -1) {
        perror("writing error");
        goto error;
    }
    if (ecrit == 0) {
        printf("server off\n");
        goto error;
    }

    printf("Receipt of server message\n");
    // receive the server message
    uint16_t server_msg[3];
    ssize_t recu = recv_bytes(clientfd,(char*)server_msg, SERVER_MESSAGE_SIZE);

    if (recu == (size_t) -1) {
        perror("reading error");
        goto error;
    }
    if (recu == 0) {
        printf("server off\n");
        goto error;
    }
    // codereq check
    request_type codereq = get_codereq_entete(server_msg[0]);
    if(!handle_codereq_error(codereq)){
        goto error;
    }
    for(int i = 0; i < 3; i++){
        server_msg[i] = ntohs(server_msg[i]);
    }
    // we call the function that sends the file in udp
    send_file_udp(file,server_msg[2], msg, server_addr);

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

    //*** creating server sockaddr ***
    struct sockaddr_in6 address_sock;
    memset(&address_sock,0,sizeof(address_sock));
    address_sock.sin6_family = AF_INET6;
    address_sock.sin6_port = htons(server_port);
    if (inet_pton(AF_INET6, server_addr, &(address_sock.sin6_addr)) <= 0) {
        printf("Unknown address %s.\n Please make sure the address is IPv6 format.\n", server_addr);
        exit(EXIT_FAILURE);
    }

    //*** connection to the server ***
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
           " |               1 - Register                                 |\n"
           " |               2 - Post a message                           |\n"
           " |               3 - List the last n messages                 |\n"
           " |               4 - Subscribe to a thread                    |\n"
           " |               5 - Upload a file                            |\n"
           " |               6 - Download a file                          |\n"
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

    int result = mkdir(directory_for_files, 0777);
    if (result == 0) {
        printf("Dossier \"%s\" créé avec succès.\n", directory_for_files);
    } else if (errno == EEXIST) {
        printf("Le dossier \"%s\" existe déjà.\n", directory_for_files);
    } else {
        perror("Erreur lors de la création du dossier");
    }

    while(1){
        print_ascii();

        while(1){
            printf("Please enter a number between 1 and 6:\n");
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

        char pseudo[11];
        int nbfil=0;
        int n=0;
        ssize_t read;

        if(res_ins==NULL && choice!=1){
            printf("\nYou need to be registered first (Press 1)\n\n");
            continue;
        }

        client();

        switch(choice){
            case REGISTER:
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
            case POST_MESSAGE:
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
            case LIST_MESSAGES:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);
                printf("Please enter the number of posts:\n");
                print_prompt();
                scanf("%d",&n);

                request_n_tickets(res_ins,nbfil,n);
                break;
            case SUBSCRIBE:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);

                subscribe_to_thread(nbfil);
                break;
            case UPLOAD_FILE:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);

                add_file(nbfil);
                break;
            case DOWNLOAD_FILE:
                printf("Please enter the thread:\n");
                print_prompt();
                scanf("%d",&nbfil);

                download_file(nbfil);
                break;
            default:
                exit(0);
        }

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
