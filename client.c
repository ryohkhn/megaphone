#include "client.h"

inscription *create_inscription(char pseudo[]){
    if(strlen(pseudo)!=10){
        char new_pseudo[10];
        strncpy(new_pseudo,pseudo,10);
        for(size_t i=strlen(new_pseudo); i<10; i++){
            new_pseudo[i]='#';
        }
        strncpy(pseudo,new_pseudo,10);
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

void send_message(res_inscription *i,char *data,int nbfil){
    client_message *msg=malloc(sizeof(client_message));

    msg->entete.val=create_entete(2,i->id)->val;
    msg->numfil=nbfil;
    uint8_t datalen=strlen(data);
    msg->data=malloc(sizeof(uint8_t)*((datalen)+1));

    *msg->data=datalen;
    memcpy(msg->data+1,data,sizeof(char)*(datalen));

    print_bits(ntohs(msg->entete.val));
    print_bits(msg->numfil);
    print_bits(msg->nb);
    for(int j=0; j<=datalen+1; ++j){
        if(j!=0)
            print_8bits(msg->data[j]);
    }

    ssize_t ecrit=send(clientfd,msg,sizeof(msg->entete)+sizeof(uint16_t)*2+sizeof(uint8_t)*(datalen+1),0);
    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }
    printf("message envoyé\n");

    //*** reception d'un message ***
    uint16_t *server_msg=malloc(sizeof(uint8_t)*((datalen)+1));
    memset(server_msg,0,sizeof(uint8_t)*((datalen)+1));

    ssize_t recu=recv(clientfd,server_msg,sizeof(uint8_t)*((datalen)+1),0);
    printf("retour du serveur reçu\n");
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }
    for(int i=0; i<3; i++){
        print_bits(ntohs((uint16_t) server_msg[i]));
    }
}

void request_n_tickets(res_inscription *i,char *data,int nbfil,int n){
    client_message *msg=malloc(sizeof(client_message));

    msg->entete.val=create_entete(3,i->id)->val;
    msg->numfil=0;
    msg->nb=0;
    msg->data=malloc(sizeof(uint8_t)*2);
    *msg->data=0;

    ssize_t ecrit=send(clientfd,msg,sizeof(msg->entete)+sizeof(uint16_t)*2+sizeof(uint8_t)*2,0);
    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }
    printf("message envoyé\n");

    //*** reception d'un message ***
    uint16_t *server_msg=malloc(sizeof(uint16_t)*1024);
    memset(server_msg,0,sizeof(uint16_t)*1024);

    ssize_t recu=recv(clientfd,server_msg,sizeof(uint16_t)*1024,0);
    printf("retour du serveur reçu\n");
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }
    for(int j=0; j<1024/16; j++){
        print_bits(ntohs((uint16_t) server_msg[j]));
    }
    //printf("Test: %s\n",server_msg[0]);
}

res_inscription *send_inscription(inscription *i){
    ssize_t ecrit=send(clientfd,i,12,0);
    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }
    printf("demande d'inscription envoyée\n");

    uint16_t server_msg[3];
    memset(server_msg,0,3*sizeof(uint16_t));

    //*** reception d'un message ***
    ssize_t recu=recv(clientfd,server_msg,3*sizeof(uint16_t),0);
    printf("retour du serveur reçu\n");
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }

    for(int j=0; j<3; j++){
        print_bits(ntohs((uint16_t) server_msg[j]));
    }

    res_inscription *res=malloc(sizeof(res_inscription));
    res->id=(ntohs(server_msg[0]))>>5;
    print_bits(res->id);

    return res;
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
    printf("Entrez un chiffre:\n");
}

res_inscription *test(){
    char pseudo[]="testlucas";
    inscription *i=create_inscription(pseudo);
    return send_inscription(i);
}


void run(){
    char input[50];
    long choice;
    char *endptr;

    res_inscription *user_id=malloc(sizeof(res_inscription));

    while(1){
        print_ascii();

        fgets(input,50,stdin);
        choice=strtol(input,&endptr,10);

        if(endptr==input || *endptr!='\n'){
            printf("Invalid input\n");
            exit(1);
        }

        client();

        char *reponse=malloc(1024*sizeof(char));
        int nbfil=0;
        int n=0;

        switch(choice){
            case 1:
                user_id=test();
                printf("%d\n",user_id->id);
                break;
            case 2:
                printf("Please enter the thread (0 for a new thread): ");
                scanf("%d",&nbfil);

                printf("Message to post: ");
                scanf("%s",reponse);

                printf("NBFIL: %d\nInput: %s\n",nbfil,reponse);

                send_message(user_id,reponse,nbfil);
                break;
            case 3:
                printf("Please enter the thread: ");
                scanf("%d",&nbfil);

                printf("Please enter the number of posts: ");
                scanf("%d",&n);

                request_n_tickets(user_id,"",nbfil,n);
                break;
            case 4:
            case 5:
            case 6:
            default:
                exit(0);
        }

        close(clientfd);
    }
}

int main(void){
    run();
    return 0;
}
