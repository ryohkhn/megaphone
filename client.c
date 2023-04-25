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
    client_message *msg = malloc(sizeof(client_message));

    msg->entete.val = create_entete(2,i->id)->val;
    msg->numfil = htons(nbfil);
    uint8_t datalen = strlen(data)+1;
    msg->data = malloc(sizeof(uint8_t)*((datalen)+1));
    *msg->data = datalen;
    memcpy(msg->data+1,data,sizeof(char)*(datalen));

    // Serialize the client_message structure and send to clientfd
    char *serialized_msg = client_message_to_string(msg);
    size_t serialized_msg_size = sizeof(uint16_t) * 3 + (datalen + 1) * sizeof(uint8_t);
    ssize_t ecrit = send(clientfd, serialized_msg, serialized_msg_size, 0);

    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }

    //*** reception d'un message ***

    uint16_t *server_msg=malloc(sizeof(uint8_t)*((datalen)+1));
    memset(server_msg,0,sizeof(uint8_t)*((datalen)+1));

    ssize_t recu=recv(clientfd,server_msg,sizeof(uint16_t)*(3),0);
    printf("retour du serveur reçu\n");
    printf("Message écrit sur le fil %d\n",ntohs(*(server_msg+1)));
    if(recu<0){
        perror("erreur lecture");
        exit(4);
    }
    if(recu==0){
        printf("serveur off\n");
        exit(0);
    }
}

char* pseudo_nohashtags(uint8_t* pseudo){
    int len = 10;
    while (len > 0 && pseudo[len-1] == '#') {
        len--;
    }

    char* str=malloc(sizeof(char)*11);
    memcpy(str, pseudo, len);
    str[len] = '\0';

    return str;
}

void print_n_tickets(char *server_msg,uint16_t numfil){
    // TODO Vérifier erreur CODEREQ 31
    server_message* received_msg=string_to_server_message(server_msg);

    printf("ID local: %d\n",user_id);
    printf("ID reçu: %d\n",get_id_entete(received_msg->entete.val));

    uint16_t nb_fil_serv=ntohs(received_msg->numfil);
    uint16_t nb_serv=ntohs(received_msg->nb);
    size_t count=6;

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

        printf("\nFil %d\n",ntohs(received_billet->numfil));
        char* originaire=pseudo_nohashtags(received_billet->origine);
        printf("Originaire du fil: %s\n\n",originaire);
        char* pseudo=pseudo_nohashtags(received_billet->pseudo);
        printf("<%s> ",pseudo);
        printf("%s\n",received_billet->data+1);

        count+=sizeof(received_billet->numfil)+sizeof(uint8_t)*10+sizeof(uint8_t)*10+sizeof(uint8_t)*(*received_billet->data+1);
        /*
        printf("\nNuméro du fil: %d\n",ntohs(chars_to_uint16(server_msg[count],server_msg[count+1])));
        count+=2;

        printf("Originaire du fil: ");
        char* originaire=malloc(sizeof(char)*10);
        memcpy(originaire,server_msg+count,10);
        printf("%s\n",originaire);
        free(originaire);

        printf("Pseudo: ");
        char* pseudo=malloc(sizeof(char)*10);
        memcpy(pseudo,server_msg+count,10);
        printf("%s\n",pseudo);
        free(pseudo);

        uint8_t datalen=server_msg[count];
        count++;
        printf("Datalen: %d\n",datalen);
        if(datalen==0){
            printf("Aucun message\n");
        }
        else{
            printf("Message:\n");
            char* message=malloc(sizeof(char)*datalen);
            message=memcpy(message,server_msg+count,datalen);
            printf("%s\n",message);
            free(message);
        }
        */
    }
    printf("\n");
}

void request_n_tickets(res_inscription *i,uint16_t numfil,uint16_t n){
    client_message *msg=malloc(sizeof(client_message));

    msg->entete.val=create_entete(3,i->id)->val;
    msg->numfil=htons(numfil);
    msg->nb=htons(n);
    msg->data=malloc(sizeof(uint8_t)*2);
    *(msg->data)=0;

    printf("\nEntête envoyée au serveur:\n");
    print_bits(ntohs(msg->entete.val));

    ssize_t ecrit=send(clientfd,msg,sizeof(msg->entete)+sizeof(uint16_t)*2+sizeof(uint8_t)*2,0);
    if(ecrit<=0){
        perror("Erreur ecriture");
        exit(3);
    }
    printf("Message envoyé au serveur.\n");

    // reception d'un message
    char* server_msg=malloc(sizeof(char)*SIZE_BUF);
    memset(server_msg,0,sizeof(char)*SIZE_BUF);

    int offset=0;
    printf("retour du serveur: \n");
    while(1){
        ssize_t recu=recv(clientfd,server_msg+offset,sizeof(char) * SIZE_BUF,0);
        if(recu<0){
            perror("erreur lecture");
            exit(4);
        }
        if(recu==0){
            printf("serveur off\n");
            break;
        }
        offset+=SIZE_BUF;

        server_msg=realloc(server_msg,sizeof(char)*(offset+SIZE_BUF));
    }

    print_n_tickets(server_msg,numfil);
}

res_inscription* send_inscription(inscription *i){
    ssize_t ecrit=send(clientfd,i,12,0);
    if(ecrit<=0){
        perror("erreur ecriture");
        exit(3);
    }
    print_bits((i->entete).val);
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
        if(j==0) printf("codereq/id:  ");
        else if (j==1) printf("numfil:      ");
        else printf("nb:          ");
        print_bits(ntohs((uint16_t) server_msg[j]));
    }

    res_inscription *res=malloc(sizeof(res_inscription));
    res->id=get_id_entete(server_msg[0]);

    return res;
}

void subscribe_to_fil(uint16_t fil_number) {
    client_message *msg = malloc(sizeof(client_message));
    msg->entete.val=create_entete(4,user_id)->val;
    msg->numfil = htons(fil_number);

    ssize_t ecrit=send(clientfd,msg,sizeof(uint16_t)*3+sizeof(uint8_t)*1,0);
    if(ecrit<=0){
        perror("Erreur ecriture");
        exit(3);
    }

    //TODO receive response from server with the multicast address

    // TODO set up a separate thread or process
    //    to listen for messages on that address

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

res_inscription *test(){
    char pseudo[]="testlucas";
    inscription *i=create_inscription(pseudo);
    return send_inscription(i);
}


void run(){
    int choice;

    res_inscription *res_ins;

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

        client();

        char *reponse=malloc(1024*sizeof(char));
        int nbfil=0;
        int n=0;

        switch(choice){
            case 1:
                res_ins=test();
                user_id=res_ins->id;
                printf("id: %d\n",res_ins->id);
                break;
            case 2:
                printf("Please enter the thread (0 for a new thread): ");
                scanf("%d",&nbfil);
                printf("Message to post: ");
                scanf("%s",reponse);
                printf("NBFIL: %d\nInput: %s\n",nbfil,reponse);

                send_message(res_ins,reponse,nbfil);
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
