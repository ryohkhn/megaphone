#include "server.h"


#define PORT 7777
#define SIZE_BUF 256

// même fonction que client.c (mettre dans utilities ou un .h/.c partagé ?)


void send_message(uint8_t codereq, uint16_t id, uint16_t nb, uint16_t numfil, int sock_client){
   server_message * msg = malloc(sizeof(server_message));

    msg->entete.val = create_entete(codereq,id)->val;
    msg->numfil = numfil;
    msg->nb = nb;

    int nboctet = send(sock_client, msg, sizeof(server_message), 0);
    if(nboctet <= 0)perror("send");



}

// todo mutex
void inscription_client(char * pseudo, int sock_client){
    list_client * current_client = clients;
    id_dernier_client += 1;
    if(current_client == NULL){
        current_client = malloc(sizeof(list_client));
        current_client->id = id_dernier_client;
        current_client->pseudo = malloc(sizeof(char) * strlen(pseudo));
        current_client->pseudo = pseudo;
        printf("Created client with pseudo: %s and id: %ld\n", current_client->pseudo, current_client->id);
    }
    else {
        // on va au dernier client de la liste
        while (current_client->suivant != NULL) {
            current_client = current_client->suivant;
        }

        // on crée le suivant
        current_client->suivant = malloc(sizeof(list_client));
        current_client = current_client->suivant;

        // on modifie le suivant
        current_client->id = id_dernier_client;
        current_client->pseudo = malloc(sizeof(char) * strlen(pseudo));
        current_client->pseudo = pseudo;
        printf("Created client with pseudo: %s and id: %ld\n", current_client->pseudo, current_client->id);
    }


    // on envoie le message de l'inscription
    send_message(1, current_client->id, 0, 0, sock_client);
}


void poster_billet(client_message *msg){
  int id = (ntohs(msg->entete.val)) >> 5;

  printf("Received message from user with id %d, to post to numfil %d\n",id,msg->numfil);
  printf("The message is: %s\n", (char *) (msg->data + 1));

  //TODO handle numfil = 0 (pick random)

  //TODO handle stocking message

  //TODO sending response to client
}

void *serve(void *arg){

// on cherche codereq pour creer la structure correspondante et appeler la bonne fonction
    int sock_client = *((int *) arg);
    char buffer[sizeof(client_message)];
    while(1) {
        int nb_octets = recv(sock_client, buffer, sizeof(buffer), 0);

        if (nb_octets < 0) {
            perror("recv serve");
            // gestion d'erreur
        } else if (nb_octets == 0) {
            printf("la connexion a été fermée");
            break;
            // la connexion a été fermée
        }
        else {
            // Vérification de la valeur de l'entête pour différencier les deux cas
            entete *header = (entete *) buffer;
            uint8_t codereq = ntohs(header->val) & 0x1F;
            printf("CODEREQ %d\n", codereq); // DEBUG
            if (codereq == 1) {
                // le message reçu est de type inscription_message
                inscription *insc = (inscription *) buffer;

                inscription_client(insc->pseudo, sock_client);
            } else {

                client_message *received_msg = string_to_client_message(buffer);


                switch (codereq) {
                    case 2:
                        poster_billet(received_msg);
                        break;
                    case 3:
                        break;
                    case 4:
                        break;
                    case 5:
                        break;
                    default:
                        perror("switch codereq");
                        break;
                }
            }
        }
    }


    /*

    int sock=*((int *) arg);
    char buf[SIZE_BUF];
    memset(buf,0,sizeof(buf));

    int recu=recv(sock,buf,SIZE_BUF,0);
    if(recu<0){
        perror("recv");
        close(sock);
        int *ret=malloc(sizeof(int));
        *ret=1;
        pthread_exit(ret);
    }

    if(recu==0){
        fprintf(stderr,"send du client nul\n");
        close(sock);
        return NULL;
    }

    printf("recu : %s\n",buf);
    char c='o';
    int ecrit=send(sock,&c,1,0);
    if(ecrit<=0)
        perror("send");


    close(sock);
    return NULL;
     */
     return NULL;
}

int main(){
    //*** creation de l'adresse du destinataire (serveur) ***
    struct sockaddr_in6 address_sock;
    memset(&address_sock,0,sizeof(address_sock));
    address_sock.sin6_family=AF_INET6;
    address_sock.sin6_port=htons(PORT);
    address_sock.sin6_addr=in6addr_any;

    //*** creation de la socket ***
    int sock=socket(PF_INET6,SOCK_STREAM,0);
    if(sock<0){
        perror("creation socket");
        exit(EXIT_FAILURE);
    }

    //*** desactiver l'option n'accepter que de l'IPv6 **
    int optval=0;
    int r_opt=setsockopt(sock,IPPROTO_IPV6,IPV6_V6ONLY,&optval,sizeof(optval));
    if(r_opt<0)
        perror("erreur connexion IPv4 impossible");

    //*** le numero de port peut etre utilise en parallele ***
    optval=1;
    r_opt=setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    if(r_opt<0)
        perror("erreur réutilisation de port impossible");

    //*** on lie la socket au port ***
    int r=bind(sock,(struct sockaddr *) &address_sock,sizeof(address_sock));
    if(r<0){
        perror("erreur bind");
        exit(EXIT_FAILURE);
    }

    //*** Le serveur est pret a ecouter les connexions sur le port ***
    r=listen(sock,0);
    if(r<0){
        perror("erreur listen");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("Erreur d'initialisation de sigaction");
        exit(EXIT_FAILURE);
    }


    while(running){
        struct sockaddr_in6 addrclient;
        socklen_t size=sizeof(addrclient);

        //*** on crée la varaiable sur le tas ***
        int *sock_client=malloc(sizeof(int));

        //*** le serveur accepte une connexion et initialise la socket de communication avec le client ***
        *sock_client=accept(sock,(struct sockaddr *) &addrclient,&size);

        if(sock_client>=0){
            pthread_t thread;
            //*** le serveur cree un thread et passe un pointeur sur socket client à la fonction serve ***
            if(pthread_create(&thread,NULL,serve,sock_client) == -1){
                perror("pthread_create");
                continue;
            }
            //*** affichage de l'adresse du client ***
            char nom_dst[INET6_ADDRSTRLEN];
            printf("client connecte : %s %d\n",inet_ntop(AF_INET6,&addrclient.sin6_addr,nom_dst,sizeof(nom_dst)),
                   htons(addrclient.sin6_port));
        }
    }
    printf("Closing sock\n");

    //*** fermeture socket serveur ***
    close(sock);

    return 0;
}
