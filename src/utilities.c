#include "../include/utilities.h"

/**
 * Function used to test if a malloc or realloc worked
 * @param ptr the allocated pointer
 */
void testMalloc(void *ptr){
    if(ptr==NULL){
        free(ptr);
        perror("malloc() or realloc() error.\n");
        exit(1);
    }
}

/**
 * Function to get the id from a header received over the network
 * @param ent the header
 * @return the id as a 16 bits integer
 */
uint16_t get_id_entete(uint16_t ent){
    return ntohs(ent)>>5;
}

/**
 * Function to get the codereq from a header received over the network
 * @param val the header
 * @return the codereq as a request_type macro
 */
request_type get_codereq_entete(uint16_t val){
    return ntohs(val) & 0x1F;
}

/**
 * Function used to create a header structure with proper bit ordering
 * @param codereq the codereq for the header
 * @param id the id for the header
 * @return an allocated header structure
 */
entete *create_entete(uint8_t codereq,uint16_t id){
    entete* entete=malloc(sizeof(struct entete));
    testMalloc(entete);
    entete->val = id;
    entete->val = (entete->val<<5) | codereq;
    entete->val = htons(entete->val);
    return entete;
}

/**
 * Function to serialize a buffer into an allocated client_message structure
 * @param buffer the buffer to serialize
 * @return the allocated structure
 */
client_message *string_to_client_message(const char *buffer) {
    client_message *msg = malloc(sizeof(client_message));
    testMalloc(msg);

    // Copy the numfil, nb and datalen
    memcpy(&(msg->numfil), buffer, NUMFIL_SIZE);
    memcpy(&(msg->nb), buffer + NUMFIL_SIZE, NB_SIZE);
    memcpy(&(msg->datalen), buffer + NUMFIL_SIZE + NB_SIZE, DATALEN_SIZE);

    return msg;
}

/**
 * Function to serialize a buffer into an allocated server_message structure
 * @param buffer the buffer to serialize
 * @return the allocated structure
 */
server_message *string_to_server_message(const char *buffer) {
    server_message *msg = malloc(sizeof(server_message));
    testMalloc(msg);

    // Copy entete, numfil, and nb from the buffer
    memcpy(&(msg->entete.val), buffer, sizeof(uint16_t));
    memcpy(&(msg->numfil), buffer + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&(msg->nb), buffer + sizeof(uint16_t) * 2, sizeof(uint16_t));

    return msg;
}

/**
 * Function to serialize a server_subscription_message structure into a buffer
 * @param buffer the structure to serialize
 * @return the serialized buffer
 */
char* server_message_to_string(server_message *msg){
    size_t buffer_size = SERVER_MESSAGE_SIZE;
    char *buffer=malloc(buffer_size);
    testMalloc(buffer);

    memcpy(buffer,&(msg->entete.val),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&(msg->numfil),sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t)*2,&(msg->nb),sizeof(uint16_t));

    return buffer;
}

/**
 * Function to serialize a server_subscription_message structure into a buffer
 * @param buffer the structure to serialize
 * @return the serialized buffer
 */
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

/**
 * Function to serialize a buffer into an allocated server_subscription_message structure
 * @param buffer the buffer to serialize
 * @return the allocated structure
 */
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

/**
 * Function to serialize a buffer into an allocated server_billet structure
 * @param buffer the buffer to serialize
 * @return the allocated structure
 */
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

/**
 * Function to serialize a client_message structure into a buffer
 * @param buffer the structure to serialize
 * @return the serialized buffer
 */
char *client_message_to_string(client_message *msg) {
    size_t buffer_size = sizeof(uint16_t) * 3 + sizeof(char) * (msg->datalen + 1);

    char *buffer = malloc(buffer_size);
    testMalloc(buffer);

    memcpy(buffer, &(msg->entete.val), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t), &(msg->numfil), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t) * 2, &(msg->nb), sizeof(uint16_t));
    memcpy(buffer + sizeof(uint16_t) * 3, &(msg->datalen), sizeof(uint8_t));
    memcpy(buffer + sizeof(uint16_t) * 3 + sizeof(uint8_t), msg->data, msg->datalen);

    return buffer;
}

/**
 * Function to serialize a buffer into an allocated notification structure
 * @param buffer the buffer to serialize
 * @return the allocated structure
 */
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

/**
 * Function to determine the size of a file
 * @param file the file name
 * @return the size of the file
 */
long size_file(FILE *file) {
    fseek(file, 0, SEEK_END);
    long taille = ftell(file);
    fseek(file, 0, SEEK_SET);
    return taille;
}

/**
 * Function that receive the file in UDP.
 * @param file_directory file where the file is wrote
 * @param port listening port
 * @param fil thread where the file is
 * @param filename file name
 */
void receive_file_udp(char * file_directory, int port, int fil, char * filename){

    // socket UDP
    int sock_udp = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("socket UDP");
        return;
    }

    // set the timeout for recvfrom to 30 seconds
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error while setting timeout");
        return;
    }
    int off = 0;
    if (setsockopt(sock_udp, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off)) < 0) {
        perror("Error setting IPV6_V6ONLY option");
        return;
    }

    // configuration of the receiver's address
    struct sockaddr_in6 addr_receveur;
    memset(&addr_receveur, 0, sizeof(addr_receveur));
    addr_receveur.sin6_family = AF_INET6;
    addr_receveur.sin6_addr = in6addr_any;
    addr_receveur.sin6_port = htons(port);

    // bind address and socket
    if (bind(sock_udp, (struct sockaddr *)&addr_receveur, sizeof(addr_receveur)) < 0) {
        perror("bind UDP");
    }

    // sender's address
    struct sockaddr_in6 addr_envoyeur;
    socklen_t addr_len = sizeof(addr_envoyeur);

    // initialization of variables
    char recv_buffer[516];
    memset(recv_buffer, 0, sizeof(char) * 516);
    int packet_num = -1;
    size_t bytes_read;
    FILE *file = NULL;
    message_udp ** received_msgs_buffer;
    int buffer_size = 0;
    int buffer_capacity = 4; // Initial size

    received_msgs_buffer = calloc( buffer_capacity, sizeof(message_udp *));
    testMalloc(received_msgs_buffer);
    // loop to listen to messages in udp
    while(1){
        // listening...
        printf("Receiving the UDP message ");
        bytes_read = recvfrom(sock_udp, recv_buffer, sizeof(char) * 512 + sizeof(u_int16_t) * 2, 0, (struct sockaddr *)&addr_envoyeur, &addr_len);

        // we check that recvfrom has timeout / the transmission is finished
        if (bytes_read == 0) {
            break;
        }
        else{
            if (bytes_read == (size_t) -1) {
                printf("Error when receiving data\n");
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    printf("recvfrom expired after 30 seconds\n");
                }
                perror("recvfrom");
                break;
            }
        }

        if (file == NULL) {
            // opening the file
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/%d_%s", file_directory, fil, filename);
            file = fopen(file_path, "w");
            if (file == NULL) {
                perror("Error opening the file");
                break;
            }
        }

        // de-serializing
        message_udp *received_msg_udp = malloc(sizeof(message_udp));
        testMalloc(received_msg_udp);
        memcpy(&(received_msg_udp->entete.val), recv_buffer, sizeof(uint16_t));
        memcpy(&(received_msg_udp->numbloc), recv_buffer + sizeof(uint16_t), sizeof(uint16_t));
        received_msg_udp->data = malloc(sizeof(char) * bytes_read - sizeof(uint16_t) * 2);
        testMalloc(received_msg_udp->data);
        memcpy(received_msg_udp->data, recv_buffer + sizeof(uint16_t) * 2, sizeof(char) * bytes_read - sizeof(uint16_t) * 2);

        // management of the case of disorder in the packages and writing in the file
        if (received_msg_udp->numbloc == packet_num + 1) {
            // if the packet that arrives is the right one (following the last one written)
            // we write it to its destination
            fwrite(received_msg_udp->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);
            packet_num = received_msg_udp->numbloc;

            // Check if the following blocks are already in the buffer
            while (packet_num + 1 < buffer_capacity && received_msgs_buffer[packet_num + 1] != NULL) {
                packet_num = received_msgs_buffer[packet_num + 1]->numbloc;
                // if the following package is already in our temporary storage buffer
                // we write it to its destination
                fwrite(received_msgs_buffer[packet_num]->data, 1, sizeof(char) * bytes_read - sizeof(uint16_t) * 2, file);
            }
        } else {
            // If the block is not the expected one, store in the array
            // Check if the capacity of the array should be increased
            while (received_msg_udp->numbloc >= buffer_capacity) {
                buffer_capacity *= 2; // Doubler la capacité
                received_msgs_buffer = realloc(received_msgs_buffer, sizeof(message_udp *) * buffer_capacity);
                testMalloc(received_msgs_buffer);
            }
            received_msgs_buffer[received_msg_udp->numbloc] = received_msg_udp;
            buffer_size = (received_msg_udp->numbloc > buffer_size) ? received_msg_udp->numbloc : buffer_size;
        }
        printf("packet n°%d (sent %zu bytes)\n",packet_num,bytes_read);

        // free
        free(received_msg_udp->data);
        free(received_msg_udp);

        // if last udp package, we break
        if(bytes_read < sizeof(char) * 512 + sizeof(u_int16_t) * 2){
            break;
        }

    }
    printf("Transmission has ended\n\n");
    // close
    if (file != NULL) {
        fclose(file);
    }
    free(received_msgs_buffer);
    close(sock_udp);
}

/**
 * Function that send the file in UDP
 * @param file the file which is sent
 * @param port listening port
 * @param msg client message
 * @param addr_IP IP adress
 */
void send_file_udp(FILE * file, int port, client_message *msg, char * addr_IP) {

    // creating UDP socket
    int sock_udp = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("Socket creation error");
        return;
    }

    int off = 0;
    if (setsockopt(sock_udp, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) < 0) {
        perror("IPV6_V6ONLY disabling error");
        close(sock_udp);
        return;
    }

    // Configuration of the receiver address
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, addr_IP, &addr.sin6_addr) <= 0) {
        perror("Error while converting the IP address");
        close(sock_udp);
        return;
    }
    socklen_t len_addr = sizeof(addr);

    // initialization of variables
    char buffer[512];
    int packet_num = 0;
    size_t bytes_read;

    message_udp * msg_udp = malloc(sizeof(message_udp));
    testMalloc(msg_udp);

    struct timespec pause;
    pause.tv_sec = 0;
    pause.tv_nsec = 2000000;

    // send loop to receiver in udp
    while ((bytes_read = fread(buffer, 1, 512, file)) > 0) {
        msg_udp->entete = msg->entete;
        msg_udp->numbloc = packet_num;
        msg_udp->data = malloc(sizeof(char) * bytes_read);
        testMalloc(msg_udp->data);
        memcpy(msg_udp->data, buffer, sizeof(char) * bytes_read);

        // we serialize
        size_t serialize_buf_size = sizeof(char) * bytes_read + sizeof(uint16_t) * 2;
        char *serialize_buffer = malloc(serialize_buf_size);
        testMalloc(serialize_buffer);
        memcpy(serialize_buffer, &(msg_udp->entete.val), sizeof(uint16_t));
        memcpy(serialize_buffer + sizeof(uint16_t), &(msg_udp->numbloc), sizeof(uint16_t));
        memcpy(serialize_buffer + sizeof(uint16_t) * 2, msg_udp->data, bytes_read);

        printf("Sending the UDP message ");
        ssize_t bytes_sent = sendto(sock_udp, serialize_buffer, serialize_buf_size, 0, (struct sockaddr *)&addr, len_addr);

        nanosleep(&pause, NULL);
        printf("packet n°%d (sent %zu bytes)\n",packet_num,bytes_sent);

        packet_num += 1;
        free(serialize_buffer);
        free(msg_udp->data);
        if (bytes_sent < 0) {
            perror("Error when sending data");
            break;
        }
    }
    printf("Transmission has ended\n\n");
    free(msg_udp);
    close(sock_udp);
}

/**
 * Function to receive exactly `len` bytes from the `sockfd` fd.
 * The function uses a loop the be TCP "safe".
 * @param sockfd the socket to recv from
 * @param buf the buffer to copy bytes into
 * @param len the number of bytes to receive
 * @return the number of bytes received or the return value of recv if an error occurs
 */
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

/**
 * Function to receive an unlimited amount of bytes from the server.
 * The function uses a loop the be TCP "safe".
 * @param sockfd the socket to recv from
 * @param buf the buffer to copy bytes into
 * @param buffer_size the initial size of the buffer
 * @return the number of bytes received or the return value of recv if an error occurs
 */
ssize_t recv_unlimited_bytes(int sockfd, char* buf, ssize_t buffer_size){
    ssize_t bytes_received;
    ssize_t total_bytes_received = 0;

    while ((bytes_received = recv(sockfd, buf + total_bytes_received, BUFSIZ, 0)) > 0) {
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

/**
 * Function that checks if the file has a valid name.
 * It uses windows standards for name detection.
 * @param filename the name of the file
 * @return a boolean, 1=vadid, 0=false
 */
int is_valid_filename(const char *filename) {
    if(strlen(filename) < 3) return 0;
    const char *invalid_characters = "/\\?%*:|\"<>";
    return strpbrk(filename, invalid_characters) == NULL;
}
