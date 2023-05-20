#include "../include/utilities.h"

//Mutex pour l'envoie d'un fichier (éviter de lire le meme fichier en même temps)
pthread_mutex_t file_reader_mutex = PTHREAD_MUTEX_INITIALIZER;

void testMalloc(void *ptr){
    if(ptr==NULL){
        free(ptr);
        perror("Erreur de malloc() ou realloc().\n");
        exit(1);
    }
}

uint16_t get_id_entete(uint16_t ent){
    return ntohs(ent)>>5;
}

request_type get_codereq_entete(uint16_t val){
    return ntohs(val) & 0x1F;
}

uint16_t chars_to_uint16(char a,char b){
    return ((uint16_t)b << 8) | a;
}

void print_8bits(uint8_t n){
    for(int i=7; i>=0; i--){
        uint8_t mask=1<<i;
        uint8_t bit=(n & mask)>>i;
        printf("%u",bit);
    }
    printf("\n");
}

void print_bits(uint16_t n){
    for(int i=0; i<=15; i++){
        uint16_t mask=1<<i;
        uint16_t bit=(n & mask)>>i;
        printf("%u",bit);
    }
    printf("\n");
}

entete *create_entete(uint8_t codereq,uint16_t id){
    entete* entete=malloc(sizeof(struct entete));
    testMalloc(entete);
    entete->val = id;
    entete->val = (entete->val<<5) | codereq;
    entete->val = htons(entete->val);
    return entete;
}

void print_inscription_bits(inscription *msg){
    uint8_t *bytes=(uint8_t *) msg;
    for(size_t i=0; i<sizeof(inscription); i++){
        for(int j=7; j>=0; j--){
            printf("%d",(bytes[i]>>j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}


client_message *string_to_client_message(const char *buffer) {
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);

    // Copy entete, numfil, and nb from the buffer
    memcpy(&(msg->entete.val), buffer, sizeof(uint16_t));
    memcpy(&(msg->numfil), buffer + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&(msg->nb), buffer + sizeof(uint16_t) * 2, sizeof(uint16_t));

    // Extract datalen from the buffer, located right after nb
    uint8_t datalen = buffer[sizeof(uint16_t) * 3];

    // Allocate memory for data
    msg->data = malloc(sizeof(uint8_t) * (datalen));
    testMalloc(msg->data);

    // Copy the data from the buffer, starting after datalen
    memcpy(msg->data, buffer + sizeof(uint16_t) * 3 + sizeof(uint8_t), sizeof(uint8_t)*(datalen));

    // Manually set the datalen as the first byte of the data array
    msg->datalen = datalen;

    return msg;
}

server_message *string_to_server_message(const char *buffer) {
    server_message *msg = malloc(sizeof(server_message));
    testMalloc(msg);

    // Copy entete, numfil, and nb from the buffer
    memcpy(&(msg->entete.val), buffer, sizeof(uint16_t));
    memcpy(&(msg->numfil), buffer + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&(msg->nb), buffer + sizeof(uint16_t) * 2, sizeof(uint16_t));

    return msg;
}

char* server_message_to_string(server_message *msg){
    size_t buffer_size = SERVER_MESSAGE_SIZE;
    char *buffer=malloc(buffer_size);
    testMalloc(buffer);

    memcpy(buffer,&(msg->entete.val),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&(msg->numfil),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t)*2,&(msg->nb),sizeof(uint16_t));

    return buffer;
}

char *server_subscription_message_to_string(server_subscription_message *msg){
    size_t buffer_size=sizeof(uint8_t)*22;
    char *buffer=malloc(buffer_size);
    testMalloc(buffer);

    memcpy(buffer,&(msg->entete.val),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&(msg->numfil),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t)*2,&(msg->nb),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t)*3,msg->addrmult,sizeof(uint8_t)*16);

    return buffer;
}

server_subscription_message *string_to_server_subscription_message(const char *buffer) {
    server_subscription_message *msg = malloc(sizeof(server_subscription_message));
    testMalloc(msg);

    // Allocate memory for the address
    msg->addrmult = malloc(sizeof(uint8_t) * 16);
    testMalloc(msg->addrmult);

    // Copy entete, numfil, and nb from the buffer
    memcpy(&(msg->entete.val), buffer, sizeof(uint16_t));
    memcpy(&(msg->numfil), buffer + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&(msg->nb), buffer + sizeof(uint16_t) * 2, sizeof(uint16_t));

    // Copy the address from buffer
    memcpy(msg->addrmult, buffer + sizeof(uint16_t) * 3, (sizeof(uint8_t) * 16));

    return msg;
}

server_billet *string_to_server_billet(const char *buffer) {
    server_billet *billet= malloc(sizeof(server_billet));
    testMalloc(billet);
    size_t offset=0;

    // Copy entete, numfil, and nb from the buffer
    memcpy(&(billet->numfil), buffer, NUMFIL_SIZE);
    offset += NUMFIL_SIZE;

    billet->origine = malloc(ORIGINE_SIZE);
    testMalloc(billet->origine);
    memcpy(billet->origine, buffer+offset, ORIGINE_SIZE);
    offset += ORIGINE_SIZE;

    billet->pseudo = malloc(PSEUDO_SIZE);
    testMalloc(billet->pseudo);
    memcpy(billet->pseudo, buffer+offset, PSEUDO_SIZE);
    offset += PSEUDO_SIZE;

    // Extract datalen from the buffer, located right after nb
    uint8_t datalen = buffer[offset];
    billet->datalen = datalen;

    // Allocate memory for data, including space for the null-terminator
    billet->data = malloc(DATALEN_SIZE*(datalen + 1));
    testMalloc(billet->data);
    offset += DATALEN_SIZE;

    // Copy the data from the buffer, starting after datalen
    memcpy(billet->data, buffer + offset, DATALEN_SIZE*(datalen + 1));

    return billet;
}

char *client_message_to_string(client_message *msg) {

    printf("\nclient_message_to_string\n");
    printf("msg->entete.val = %d\n", msg->entete.val);
    printf("datalen = %d\n",msg->datalen);
    printf("msg->data = %s\n", msg->data);

    size_t buffer_size = sizeof(uint16_t) * 3 + sizeof(char) * (msg->datalen + 1);

    char *buffer = malloc(buffer_size);
    testMalloc(buffer);

    memcpy(buffer, &(msg->entete.val), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t), &(msg->numfil), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t) * 2, &(msg->nb), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t) * 3, &(msg->datalen), sizeof(uint8_t));
    memcpy(buffer + sizeof(uint16_t) * 3 + sizeof(uint8_t), msg->data, msg->datalen);

    client_message * test = string_to_client_message(buffer);
    printf("test->entete.val = %d\n", test->entete.val);
    printf("datalen = %d\n",test->datalen);
    printf("test->data = %s\n", test->data);
    printf("fin client_message_to_string\n\n");


    return buffer;
}

notification *string_to_notification(const char *buffer){
    notification *notification=malloc(sizeof(server_billet));
    testMalloc(notification);

    // Copy entete and numfil from the buffer
    memcpy(&(notification->entete),buffer,sizeof(uint16_t));
    memcpy(&(notification->numfil),buffer+sizeof(uint16_t),sizeof(uint16_t));

    // Allocate memory for pseudo
    notification->pseudo=calloc(10,sizeof(char));
    memcpy(notification->pseudo,buffer+sizeof(uint16_t)*2,sizeof(uint8_t)*10);

    // Allocate memory for data
    notification->data=calloc(20,sizeof(char));
    memcpy(notification->data,buffer+sizeof(uint16_t)*2+sizeof(char)*10,20);

    return notification;
}

client_message *copy_client_message(client_message *msg) {
    client_message *copy = malloc(sizeof(client_message));
    testMalloc(copy);

    copy->entete = msg->entete;
    copy->numfil = msg->numfil;
    copy->nb = msg->nb;
    uint8_t datalen = msg->datalen;
    copy->data = malloc(sizeof(uint8_t) * (datalen));
    testMalloc(copy->data);
    memcpy(copy->data, msg->data, sizeof(uint8_t) * (datalen));
    return copy;
}

long size_file(FILE *file) {
    fseek(file, 0, SEEK_END);
    long taille = ftell(file);
    fseek(file, 0, SEEK_SET);
    return taille;
}

void handle_error(int codereq){
    switch(codereq){
        case 31:
            printf("Erreur envoyée par le serveur\n");
            break;
    }
}

// fonction pour affichier un tableau d'entiers
void print_array(int *array, int size) {
    printf("print_array\n");
    printf("Le tableau : [");
    for (int i = 0; i < size; i++) {
        printf("%d", array[i]);
        if (i < size - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

void receive_file_udp(char * file_directory, int port, int fil, char * filename){
    printf("\n\nboucle ecoute udp\n\n");
    printf("file_directory = %s\n",file_directory);
    printf("filename = %s\n",filename);
    printf("port = %d\n",port);
    printf("fil = %d\n",fil);

    printf("Création du socket UDP\n");
    // création socket UDP IPV4
    int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("socket UDP");
    }

    printf("Configuration du délai d'attente\n");
    // on définit le délai d'attente de recvfrom à 30 secondes
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    printf("setsockopt\n");
    if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erreur lors de la définition du délai d'attente");
        return;
    }
    int off = 0;
    if (setsockopt(sock_udp, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off)) < 0) {
        perror("Erreur lors de la définition de l'option IPV6_V6ONLY");
        return;
    }

    // configuration de l'adresse du receveur
    printf("Configuration de l'adresse du receveur\n");

    struct sockaddr_in6 addr_receveur;
    memset(&addr_receveur, 0, sizeof(addr_receveur));
    addr_receveur.sin6_family = AF_INET6;
    addr_receveur.sin6_addr = in6addr_any;
    addr_receveur.sin6_port = htons(port);

    // bind de l'adresse et de la socket
    printf("bind socket udp avec option temps avec addr_receveur\n");
    if (bind(sock_udp, (struct sockaddr *)&addr_receveur, sizeof(addr_receveur)) < 0) {
        perror("bind UDP");
    }


    // obtenue avec recvfrom(......, &addr_envoyeur,....)
    struct sockaddr_in6 addr_envoyeur;
    socklen_t addr_len = sizeof(addr_envoyeur);

    // initialisation des variables
    char recv_buffer[516];
    memset(recv_buffer, 0, sizeof(char) * 516);
    int packet_num = -1;
    size_t bytes_read;
    FILE *file = NULL;
    message_udp ** received_msgs_buffer;
    int buffer_size = 0;
    int buffer_capacity = 4; // Taille initiale

    received_msgs_buffer = calloc( buffer_capacity, sizeof(message_udp *));
    testMalloc(received_msgs_buffer);
    printf("on écoute sur :\n");
    printf("sock = %d\n", sock_udp);
    printf("\n\ndébut boucle do while\n\n");
    // boucle pour écouter les messages en udp
    while(1){
        printf("Attente de données\n");
        // on écoute
        bytes_read = recvfrom(sock_udp, recv_buffer, sizeof(char) * 512 + sizeof(u_int16_t) * 2, 0, (struct sockaddr *)&addr_envoyeur, &addr_len);
        printf("Données reçues : %zu\n", bytes_read);

        // on vérifie que recvfrom a timeout / la transmission est finie
        if (bytes_read == 0) {
            printf("fin communication\n");
            break;
        }else{
            if (bytes_read == (size_t) -1) {
                printf("Erreur lors de la réception des données\n");
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    printf("recvfrom a expiré après 30 secondes\n");
                }
                perror("recvfrom");
                break;
            }
        }

        // on ouvre le FILE
        printf("ouverture du file\n");
        if (file == NULL) {
            printf("1ere iter : on crée file_path et on ouvre le FILE\n");
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%d_%s", file_directory, fil, filename);
            printf("file_path = %s\n", file_path);
            file = fopen(file_path, "w");
            if (file == NULL) {
                perror("Erreur d'ouverture du fichier");
                break;
            }
        }

        // on deserialize
        printf("on deserialize\n");
        message_udp *received_msg_udp = malloc(sizeof(message_udp));
        testMalloc(received_msg_udp);
        memcpy(&(received_msg_udp->entete.val), recv_buffer, sizeof(uint16_t));
        memcpy(&(received_msg_udp->numbloc), recv_buffer + sizeof(uint16_t), sizeof(uint16_t));
        received_msg_udp->data = malloc(sizeof(char) * bytes_read - sizeof(uint16_t) * 2);
        testMalloc(received_msg_udp->data);
        memcpy(received_msg_udp->data, recv_buffer + sizeof(uint16_t) * 2, sizeof(char) * bytes_read - sizeof(uint16_t) * 2);

        printf("received_msg_udp->entete.val = %d\n",received_msg_udp->entete.val);
        printf("received_msg_udp->numbloc = %d\n",received_msg_udp->numbloc);
        printf("received_msg_udp->data = %s\n",received_msg_udp->data);

        // gestion du cas de désordre des paquets et écriture dans le file
        printf("gestion de désordre dans les paquets et écriture dans le file\n");
        if (received_msg_udp->numbloc == packet_num + 1) {
            printf("1\n");
            // si le paquet qui arrive est le bon (suivant le dernier écrit)
            // on l'écrit a sa destination
            fwrite(received_msg_udp->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);
            packet_num = received_msg_udp->numbloc;
            printf("2\n");


            // Vérifier si les blocs suivants sont déjà dans le buffer
            while (packet_num + 1 < buffer_capacity && received_msgs_buffer[packet_num + 1] != NULL) {
                printf("3\n");
                packet_num = received_msgs_buffer[packet_num + 1]->numbloc;
                // si le paquet suivant est deja dans notre buffer de stockage temporaire
                // on l'écrit a sa destination
                fwrite(received_msgs_buffer[packet_num]->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);
                printf("4\n");

            }
            printf("5\n");

        } else {
            printf("6\n");

            // Si le bloc n'est pas le bloc attendu, stocker dans le tableau
            // Vérifier si la capacité du tableau doit être augmentée
            while (received_msg_udp->numbloc >= buffer_capacity) {
                printf("7\n");

                buffer_capacity *= 2; // Doubler la capacité
                received_msgs_buffer = realloc(received_msgs_buffer, sizeof(message_udp *) * buffer_capacity);
                testMalloc(received_msgs_buffer);
                printf("8\n");

            }
            printf("9\n");

            received_msgs_buffer[received_msg_udp->numbloc] = received_msg_udp;
            buffer_size = (received_msg_udp->numbloc > buffer_size) ? received_msg_udp->numbloc : buffer_size;
            printf("10\n");

        }

        // on free
        free(received_msg_udp->data);
        free(received_msg_udp);

        // si dernier paquet udp, on break
        if(bytes_read < sizeof(char) * 512 + sizeof(u_int16_t) * 2){
            break;
        }

    }
    printf("\nfin de while\n");
    // on close le fichier
    if (file != NULL) {
        fclose(file);
    }
    printf("on free les pointeurs\n");
    // on free les pointeurs
    for (int i = 0; i <= buffer_size; i++) {
        if (received_msgs_buffer[i] != NULL) {
            free(received_msgs_buffer[i]->data);
            free(received_msgs_buffer[i]);
        }
    }
    printf("close socket et received_msgs_buffer\n");
    free(received_msgs_buffer);
    close(sock_udp);
    printf("\nfin de fonction\n");

}

void send_file_udp(FILE * file, int port, client_message *msg) {
    msg->numfil = ntohs(msg->numfil);
    printf("\n\nboucle envoie udp\n\n");
    printf("FILE = %p\n", file);
    printf("port = %d\n", port);
    printf("msg->entete.val = %d\n", msg->entete.val);
    printf("msg->nb = %d\n", msg->nb);
    printf("msg->numfil = %d\n", msg->numfil);
    printf("msg->datalen = %d\n",msg->datalen);
    printf("msg->data = %s\n", msg->data);
    printf("\n\nCréation et configuration du socket UDP\n");
    // Création du socket IPV4 UDP
    int sock_udp = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("Erreur de création du socket");
        return;
    }

    // Configuration de l'adresse du receveur
    printf("configuration adresse du receveur\n");
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    // pour l'instant en local
    printf("pour l'instant local : ::1\n");
    //todo utiliser adresse mise en argument de la fonction
    if (inet_pton(AF_INET6, "::1", &addr.sin6_addr) <= 0) {
        perror("Erreur lors de la conversion de l'adresse IP");
        close(sock_udp);
        return;
    }
    socklen_t len_addr = sizeof(addr);

    printf("Envoi du fichier au serveur via UDP\n");
    printf("on écoute sur :\n");
    printf("sock = %d\n", sock_udp);
    char buffer[512];
    int packet_num = 0;
    size_t bytes_read;

    message_udp * msg_udp = malloc(sizeof(message_udp));
    testMalloc(msg_udp);

    printf("\n\ndébut while\n");
    // boucle d'envoie au receveur en udp
    pthread_mutex_lock(&file_reader_mutex);
    while ((bytes_read = fread(buffer, 1, 512, file)) > 0) {
        printf("Préparation du message UDP\n");
        msg_udp->entete = msg->entete;
        msg_udp->numbloc = packet_num;
        msg_udp->data = malloc(sizeof(char) * bytes_read);
        testMalloc(msg_udp->data);
        memcpy(msg_udp->data, buffer, sizeof(char) * bytes_read);
        printf("msg_udp->entete = %d\n",msg_udp->entete.val);
        printf("msg_udp->numbloc = %d\n",msg_udp->numbloc);
        printf("msg_udp->data = %s\n",msg_udp->data);


        // on serialize
        printf("\n on serialize\n");
        size_t serialize_buf_size = sizeof(char) * bytes_read + sizeof(uint16_t) * 2;
        char *serialize_buffer = malloc(serialize_buf_size);
        testMalloc(serialize_buffer);
        memcpy(serialize_buffer, &(msg_udp->entete.val), sizeof(uint16_t));
        memcpy(serialize_buffer + sizeof(uint16_t), &(msg_udp->numbloc), sizeof(uint16_t));
        memcpy(serialize_buffer + sizeof(uint16_t) * 2, msg_udp->data, bytes_read);

        printf("Envoi du message UDP\n");
        ssize_t bytes_sent = sendto(sock_udp, serialize_buffer, serialize_buf_size, 0, (struct sockaddr *)&addr, len_addr);
        printf("données envoyées = %zu\n", bytes_sent);

        packet_num += 1;

        // les free
        free(serialize_buffer);
        free(msg_udp->data);


        if (bytes_sent < 0) {
            perror("Erreur lors de l'envoi des données");
            break;
        }
    }
    pthread_mutex_unlock(&file_reader_mutex);

    printf("\nNettoyage et fermeture du fichier\n");
    free(msg_udp);
    close(sock_udp);
    printf("\nfin de fonction\n");
}

ssize_t recv_bytes(int sockfd, char *buf, ssize_t len){
    ssize_t bytes_left = len;
    size_t offset = 0;

    while(bytes_left > 0){
        ssize_t read = recv(sockfd, buf + offset, bytes_left, 0);
        if(read < 0 || read == (size_t) -1){
            return read;
        }

        bytes_left -= read;
        offset += read;
    }

    return len;
}

ssize_t recv_unlimited_bytes(int sockfd, char* buf, ssize_t buffer_size){
    ssize_t bytes_received;
    ssize_t total_bytes_received = 0;

    while ((bytes_received = recv(sockfd, buf + total_bytes_received, BUFSIZ, 0)) > 0) {
        printf("Octets reçus du serveur: %zu\n",bytes_received);
        total_bytes_received += bytes_received;

        // Check if the remaining buffer space is less than BUFSIZ
        if (buffer_size - total_bytes_received < BUFSIZ) {
            // Increase the buffer size by BUFSIZ
            buffer_size += BUFSIZ;

            // Reallocate the buffer to the new size
            char *new_buffer = realloc(buf, buffer_size);
            testMalloc(new_buffer);
            buf = new_buffer;
        }
    }
    if (bytes_received < 0 || bytes_received == (size_t) -1) {
        return bytes_received;
    }

    return total_bytes_received;
}

// selon les standards de windows
// 1 = valide, 0 = invalide
int is_valid_filename(const char *filename) {
    if(strlen(filename) < 3) return 0;
    const char *invalid_characters = "/\\?%*:|\"<>";
    return strpbrk(filename, invalid_characters) == NULL;
}
