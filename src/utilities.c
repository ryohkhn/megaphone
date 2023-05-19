#include "../include/utilities.h"

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

    // Copy entete, numfil, and nb from the buffer
    memcpy(&(billet->numfil), buffer, sizeof(uint16_t));

    billet->origine = malloc(sizeof(uint8_t)*10);
    testMalloc(billet->origine);
    memcpy(billet->origine, buffer+sizeof(uint16_t), sizeof(uint8_t)*10);

    billet->pseudo = malloc(sizeof(uint8_t)*10);
    testMalloc(billet->pseudo);
    memcpy(billet->pseudo, buffer+sizeof(uint16_t)*6, sizeof(uint8_t)*10);

    // Extract datalen from the buffer, located right after nb
    uint8_t datalen = buffer[sizeof(uint16_t) * 11];
    billet->datalen = datalen;

    // Allocate memory for data, including space for the null-terminator
    billet->data = malloc(sizeof(uint8_t)*(datalen + 1));
    testMalloc(billet->data);

    // Copy the data from the buffer, starting after datalen
    memcpy(billet->data, buffer + sizeof(uint8_t) * 23, sizeof(uint8_t)*(datalen + 1));


    return billet;
}

char *client_message_to_string(client_message *msg) {
/*
    printf("\nclient_message_to_string\n");
    printf("msg->entete.val = %d\n", msg->entete.val);
    printf("datalen = %d\n",msg->datalen);
    printf("msg->data = %s\n", msg->data);
    */

    size_t buffer_size = sizeof(uint16_t) * 3 + sizeof(char) * (msg->datalen + 1);

    char *buffer = malloc(buffer_size);
    testMalloc(buffer);

    memcpy(buffer, &(msg->entete.val), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t), &(msg->numfil), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t) * 2, &(msg->nb), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t) * 3, &(msg->datalen), sizeof(uint8_t));
    memcpy(buffer + sizeof(uint16_t) * 3 + sizeof(uint8_t), msg->data, 1 + msg->datalen);
/*
    client_message * test = string_to_client_message(buffer);
    printf("test>entete.val = %d\n", test->entete.val);
    printf("datalen = %d\n",test->datalen);
    printf("test->data = %d%s\n", test->datalen, test->data);
    printf("fin client_message_to_string\n\n");
    */

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
    uint8_t datalen = msg->data[0];
    copy->data = malloc(sizeof(uint8_t) * (datalen + 1));
    memcpy(copy->data, msg->data, sizeof(uint8_t) * (datalen + 1));
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

void boucle_ecoute_udp(char * file_directory, int sock, int fil, char * filename){
    printf("file_directory = %s\n",file_directory);
    printf("sock udp = %d\n",sock);
    printf("fil = %d\n",fil);
    printf("filename = %s\n",filename);

    printf("Écoute de la réponse\n");
    //  on écoute la réponse du client qu'on écrit dans un buffer
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // initialisation des variables
    char recv_buffer[516];
    memset(recv_buffer, 0, sizeof(char) * 516);
    int packet_num = -1;
    size_t bytes_read;
    FILE *file = NULL;
    message_udp ** received_msgs_buffer;
    int buffer_size = 0;
    int buffer_capacity = 4; // Taille initiale

    received_msgs_buffer = malloc(sizeof(message_udp *) * buffer_capacity);

    // boucle pour écouter les messages en udp
    do {
        printf("Attente de données\n");
        // on écoute
        bytes_read = recvfrom(sock, recv_buffer, sizeof(char) * 512 + sizeof(u_int16_t) * 2, 0, (struct sockaddr *)&addr, &addr_len);
        printf("Données reçues : %zu\n", bytes_read);
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

        // on deserialize
        message_udp *received_msg_udp = malloc(sizeof(message_udp));
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
            snprintf(file_path, sizeof(file_path), "%s/%d_%s", file_directory, fil, filename);
            printf("file_path = %s\n", file_path);
            file = fopen(file_path, "w");
            if (file == NULL) {
                perror("Erreur d'ouverture du fichier");
                return;
            }
        }
        // gestion du cas de désordre des paquets et écriture dans le file
        if (received_msg_udp->numbloc == packet_num + 1) {
            // si le paquet qui arrive est le bon (suivant le dernier écrit)
            // on l'écrit a sa destination
            fwrite(received_msg_udp->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);
            packet_num = received_msg_udp->numbloc;

            // Vérifier si les blocs suivants sont déjà dans le buffer
            while (packet_num + 1 < buffer_capacity && received_msgs_buffer[packet_num + 1] != NULL) {
                packet_num = received_msgs_buffer[packet_num + 1]->numbloc;
                // si le paquet suivant est deja dans notre buffer de stockage temporaire
                // on l'écrit a sa destination
                fwrite(received_msgs_buffer[packet_num]->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);
            }
        } else {
            // Si le bloc n'est pas le bloc attendu, stocker dans le tableau
            // Vérifier si la capacité du tableau doit être augmentée
            while (received_msg_udp->numbloc >= buffer_capacity) {
                buffer_capacity *= 2; // Doubler la capacité
                received_msgs_buffer = realloc(received_msgs_buffer, sizeof(message_udp *) * buffer_capacity);
            }
            received_msgs_buffer[received_msg_udp->numbloc] = received_msg_udp;
            buffer_size = (received_msg_udp->numbloc > buffer_size) ? received_msg_udp->numbloc : buffer_size;
        }
        // si dernier paquet udp, on break
        if(bytes_read < sizeof(char) * 512 + sizeof(u_int16_t) * 2) break;
    } while (1);

    // on close le fichier
    if (file != NULL) {
        fclose(file);
    }

    // on free les pointeurs
    for (int i = 0; i <= buffer_size; i++) {
        if (received_msgs_buffer[i] != NULL) {
            free(received_msgs_buffer[i]->data);
            free(received_msgs_buffer[i]);
        }
    }
    free(received_msgs_buffer);
}

void boucle_envoie_udp(FILE * file, int port, client_message *msg) {

    printf("Création et configuration du socket UDP\n");
    // Création du socket IPV4 UDP
    int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("Erreur de création du socket");
        return;
    }

    // Configuration de l'adresse du receveur
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // pour l'instant en local
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr) <= 0) {
        perror("Erreur lors de la conversion de l'adresse IP");
        close(sock_udp);
        return;
    }
    socklen_t len_addr = sizeof(addr);

    printf("Envoi du fichier au serveur via UDP\n");
    char buffer[512];
    int packet_num = 0;
    size_t bytes_read;

    message_udp * msg_udp = malloc(sizeof(message_udp));

    // boucle d'envoie au receveur en udp
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
        ssize_t bytes_sent = sendto(sock_udp, serialize_buffer, serialize_buf_size, 0, (struct sockaddr *)&addr, len_addr);
        printf("données reçues = %zu\n", bytes_sent);

        packet_num += 1;
        free(serialize_buffer);

        if (bytes_sent < 0) {
            free(msg_udp);
            perror("Erreur lors de l'envoi des données");
            return;
        }
    }

    printf("Nettoyage et fermeture du fichier\n");
    free(msg_udp);
    close(sock_udp);
}
