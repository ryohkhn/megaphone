#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int clientfd;

typedef struct entete {
    uint16_t val;
} entete;

typedef struct inscription_message {
    entete entete;
    char pseudo[10];
} inscription;

struct client_message {
    entete entete;
    uint16_t numfil;
    uint16_t nb;
    uint16_t datalen;
    char data[];
};

typedef struct server_message{
    entete entete;
    uint16_t numfil;
    uint16_t nb;
}server_message;

void testMalloc(void *ptr){
    if(ptr==NULL){
        perror("Erreur de malloc() ou realloc().\n");
        exit(1);
    }
}

void print_bits(uint16_t n){
    for (int i = 15; i >= 0; i--) {
        uint16_t mask = 1 << i;
        uint16_t bit = (n & mask) >> i;

        printf("%u", bit);
    }
    printf("\n");
}

entete* create_entete(uint8_t codereq, uint16_t id) {
    entete* entete = malloc(sizeof(entete));
    entete->val = id;
    entete->val = entete->val << 5;
    entete->val = entete->val | codereq;
    entete->val = htons(entete->val);

    return entete;
}

inscription *create_inscription(char pseudo[]){
    if(strlen(pseudo)!=10){
        char new_pseudo[10];
        strncpy(new_pseudo,pseudo,10);
        for(size_t i=strlen(new_pseudo); i<10; i++){
            new_pseudo[i]='#';
        }
        pseudo=new_pseudo;
    }

    inscription *inscription_message=malloc(sizeof(inscription));
    testMalloc(inscription_message);

    entete *entete=create_entete(1,0);
    testMalloc(entete);

    memcpy(&(inscription_message->entete),entete,sizeof(*entete));
    strncpy(inscription_message->pseudo,pseudo,10);

    free(entete);
    return inscription_message;
}

void print_inscription_bits(inscription* msg) {
    uint8_t* bytes = (uint8_t*) msg;
    for (size_t i = 0; i < sizeof(inscription); i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (bytes[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}

void send_inscription(inscription *i){
    int ecrit = send(clientfd, i, 12, 0);
    if(ecrit <= 0){
        perror("erreur ecriture");
        exit(3);
    }
    printf("demande d'inscription envoyée\n");

    uint16_t server_msg[3];
    memset(server_msg,0,3*sizeof(uint16_t));
    print_bits(ntohs((uint16_t) server_msg[0]));
    print_bits(ntohs((uint16_t) server_msg[1]));
    print_bits(ntohs((uint16_t) server_msg[2]));

    //*** reception d'un message ***
    int recu = recv(clientfd, server_msg,3*sizeof(uint16_t), 0);
    printf("retour du serveur reçu\n");
    if (recu < 0){
        perror("erreur lecture");
        exit(4);
    }
    if (recu == 0){
        printf("serveur off\n");
        exit(0);
    }
    print_bits(ntohs((uint16_t) server_msg[0]));
    print_bits(ntohs((uint16_t) server_msg[1]));
    print_bits(ntohs((uint16_t) server_msg[2]));
}

void client(){
    int fdsock = socket(PF_INET, SOCK_STREAM, 0);
    if(fdsock == -1){
        perror("creation socket");
        exit(1);
    }

    //*** creation de l'adresse du destinataire (serveur) ***
    struct sockaddr_in address_sock;
    memset(&address_sock, 0,sizeof(address_sock));
    address_sock.sin_family = AF_INET;
    address_sock.sin_port = htons(7777);
    inet_pton(AF_INET, "localhost", &address_sock.sin_addr);

    //*** demande de connexion au serveur ***
    int r = connect(fdsock, (struct sockaddr *) &address_sock, sizeof(address_sock));
    if(r == -1){
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
    printf("Entrez un chiffre:\n");
}

void test(){
    char pseudo[]="testlucas";
    inscription *i=create_inscription(pseudo);

    send_inscription(i);

    // printf("%d %d %s \n",i->entete.codereq,i->entete.id,i->pseudo); // 1 0 test######
}


void run(){
    char input[50];
    long choice;
    char *endptr;

    fgets(input, 50, stdin);
    choice = strtol(input, &endptr, 10);

    if (endptr == input || *endptr != '\n') {
        printf("Invalid input\n");
        exit(1);
    }

    client();
    switch(choice){
        case 1:
            test();
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        default:
            exit(0);
    }
    close(clientfd);
}

int main(void){
    print_ascii();
    run();
    return 0;
}
