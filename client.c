#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int clientfd;

typedef struct entete {
    uint16_t codereq : 5 ;
    uint16_t id : 11;
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

entete* create_entete(uint8_t codereq, uint16_t id) {
    //TODO make sure it's big endian...
    entete* entete = malloc(sizeof(entete));
    entete->codereq = (codereq & 0x1F) << 3; // keep only the 5 least significant bits of codereq
    entete->id = htons(id) >> 5;

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
    printf("demande d'inscription envoyée");

    char server_msg[3];
    memset(server_msg,0,3);

    //*** reception d'un message ***
    int recu = recv(clientfd, server_msg,3, 0);
    printf("retour du serveur reçu");
    if (recu < 0){
        perror("erreur lecture");
        exit(4);
    }
    if (recu == 0){
        printf("serveur off\n");
        exit(0);
    }
    printf("%s\n",server_msg);
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
    inet_pton(AF_INET, "192.168.70.237", &address_sock.sin_addr);

    //*** demande de connexion au serveur ***
    int r = connect(fdsock, (struct sockaddr *) &address_sock, sizeof(address_sock));
    if(r == -1){
        perror("echec de la connexion");
        exit(2);
    }

    clientfd=fdsock;
}


void test(){
    //struct entete entete;
    // printf("%ld \n",sizeof(entete)); // 2 octets

    char pseudo[]="testlucas";
    inscription *i=create_inscription(pseudo);

    send_inscription(i);

    // printf("%d %d %s \n",i->entete.codereq,i->entete.id,i->pseudo); // 1 0 test######

    // print_inscription_bits(i);

}

int main(void){
    client();
    test();
    close(clientfd);
    return 0;
}
