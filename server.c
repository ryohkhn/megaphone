#include "server.h"


#define PORT 7777
#define SIZE_BUF 256

// même fonction que client.c (mettre dans utilities ou un .h/.c partagé ?)


void send_message(uint8_t codereq, uint16_t id, uint16_t nb, uint16_t numfil, int sock_client){
   server_message * msg = malloc(sizeof(server_message));

    msg->entete.val = create_entete(codereq,id)->val;
    msg->numfil = htons(numfil);
    msg->nb = htons(nb);

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


void poster_billet(client_message *msg, int sock_client){
  int id = ntohs(msg->entete.val) >> 5;

  printf("Received message from user with id %d, to post to numfil %d\n",id,ntohs(msg->numfil));
  printf("The message is: %s\n", (char *) (msg->data + 1));

  uint16_t numfil = ntohs(msg->numfil);

  if (numfil == 0) {
       // Find the first empty fil
       for (uint16_t i = 1; i < MAX_FIL; ++i) {
           if (fils[i] == NULL) {
               numfil = i;
               break;
           }
       }
       printf("Assigned new numfil = %d\n", numfil);
   }

  add_message_to_fil(fils, msg, numfil);

  //TEST print all messages in the fil after adding the new one
  char** res = retrieve_messages_from_fil(fils, numfil);
  int i = 0;
  while (res[i] != NULL) {
      printf("Message %d: %s\n", i+1, res[i]);
      i++;
  }

  //sending response to client
  send_message(2, id, 0, numfil, sock_client);
}

void demander_liste_billets(client_message *msg, int sock_client){

}


void send_fil_notification(fil *current_fil) {
    if(current_fil->head == NULL) return;
    printf("DEBUG in send_fil_notification %d.\n", current_fil->subscribed);
    // Create a socket for sending multicast notifications
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    // Set the multicast address and port
    struct sockaddr_in6 dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET6, (char*) current_fil->addrmult, &dest_addr.sin6_addr);

    // Send the notification messages
    message_node *current_message = current_fil->head;
    message_node *last_multicasted_message = current_fil->last_multicasted_message;

    while (current_message != NULL && current_message->msg != NULL && current_message != last_multicasted_message){
        // Send the message
        printf("DEBUG In boucle\n");
        ssize_t message_size = sizeof(entete) + 2 * sizeof(uint16_t);
        uint8_t lendata = current_message->msg->data[0];
        message_size += lendata * sizeof(uint8_t);

        if (sendto(sockfd, current_message->msg, message_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto");
        }

        current_fil->last_multicasted_message = current_message;
        current_message = current_message->next;
    }
    close(sockfd);
}

// Fonction qui vérifie un par un les fils, si il y a besoin d'envoyer
// des messages (par exemple il y a au moins un abonné et il y a au moins un message),
// on lance la fonction send_fil_notification
void* send_notifications(void *arg) {
    fil **fils = (fil **)arg;
    // Initialise the fils
    for (int i = 0; i < MAX_FIL; i++) {
      fils[i]->fil_number = i;
      fils[i]->subscribed = 0;
   }

    while (1) {
      for (int i = 0; i < MAX_FIL; i++) {
        if ( fils[i]->head->msg != NULL &&  fils[i]->subscribed > 0) {
          // Send notifications for the fil
          printf("Send fil notif to fil %d\n",i);
          send_fil_notification(fils[i]);
        }
      }
      //printf("DEBUG \n");
      sleep(NOTIFICATION_INTERVAL);
    }

    return NULL;
}


void add_subscription_to_fil(client_message *received_msg, int sock_client){
  uint16_t numfil = ntohs(received_msg->numfil);
  int id = ntohs(received_msg->entete.val) >> 5;

  //Prepare the multicast address: start at base and add numfil
  const char *base_address = "ff02::1:1";
  struct in6_addr multicast_address;

  if (inet_pton(AF_INET6, base_address, &multicast_address) <= 0) {
      perror("inet_pton");
      return;
  }

  // Increment the last 32 bits by the numfil
  uint32_t *derniers_4_octets = (uint32_t *)(&multicast_address.s6_addr32[3]);
  *derniers_4_octets = htonl(ntohl(*derniers_4_octets) + numfil);

  char address_str[INET6_ADDRSTRLEN];
  if (inet_ntop(AF_INET6, &multicast_address, address_str, INET6_ADDRSTRLEN) == NULL) {
      perror("inet_ntop");
      return;
  }

  uint8_t* addresse_a_envoyer = malloc(sizeof(uint8_t) * 16);

  // check if fil already has a multicast address
  if (strlen(fils[numfil]->addrmult) > 0){
    printf("Multicast for this fil is already initialised.\n");
    addresse_a_envoyer = (uint8_t*) fils[numfil]->addrmult;
  }
  else{
    printf("INITIALISING MULTICAST ADDR.\n");
    // if fil does not have a multicast address then initialise it
    int multicast_sock;
    if((multicast_sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("erreur socket");
      return;
    }

    // Initialisation de l'adresse d'abonnement
    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, address_str, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(MULTICAST_PORT); //TODO pick port?

    // initialisation de l'interface locale autorisant le multicast IPv6
    int ifindex = 0; //if_nametoindex("eth0");
    if(setsockopt(multicast_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex))) {
      perror("erreur initialisation de l'interface locale");
      return;
    }
    // bind the socket
    bind(multicast_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    addresse_a_envoyer = malloc(sizeof(uint8_t) * 16); // Allocate memory for the address
    memcpy(addresse_a_envoyer, server_addr.sin6_addr.s6_addr, sizeof(uint8_t) * 16); // Copy the address bytes

    // Allocate memory for fils[numfil]->addrmult if it's not already allocated
    if (fils[numfil]->addrmult == NULL) {
      fils[numfil]->addrmult = malloc(sizeof(uint8_t) * 16);
    }

    // Copy the content of addresse_a_envoyer into fils[numfil]->addrmult
    memcpy(fils[numfil]->addrmult, server_addr.sin6_addr.s6_addr, sizeof(uint8_t) * 16);
  }

  // update fil subscription counter
  fils[numfil]->subscribed += 1;

  //sending response to client
  server_subscription_message * msg = malloc(sizeof(server_subscription_message));
  msg->entete.val = create_entete(4,id)->val;
  msg->numfil = htons(numfil);
  msg->nb = htons(MULTICAST_PORT);
  msg->addrmult = malloc(sizeof(uint8_t) * 16);
  memcpy(msg->addrmult, addresse_a_envoyer, sizeof(uint8_t) * 16);

  print_bits(msg->entete.val);
  print_bits(msg->numfil);
  print_bits(msg->nb);
  for (int i = 0; i < 16; i++) {
    for (int j = 7; j >= 0; j--) {
      printf("%d", (msg->addrmult[i] >> j) & 1);
    }
    printf("%s", (i < 15) ? ":" : "\n");
  }

  // Serialize the client_message structure and send to clientfd
  char *serialized_msg = server_subscription_message_to_string(msg);
  size_t serialized_msg_size = sizeof(uint8_t) * 22;
  int nboctet = send(sock_client, serialized_msg, serialized_msg_size, 0);
  printf("DEBUG SENT %d OCTETS\n", nboctet);
  if(nboctet <= 0)perror("send");
}

void *serve(void *arg){

// on cherche codereq pour creer la structure correspondante et appeler la bonne fonction
    int sock_client = *((int *) arg);
    char buffer[sizeof(client_message)];
    while(1) {
        int nb_octets = recv(sock_client, buffer, sizeof(buffer), 0);

        if (nb_octets < 0) {
            perror("recv serve");
            exit(1);
            // gestion d'erreur
        } else if (nb_octets == 0) {
            printf("la connexion a été fermée\n");
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
                        poster_billet(received_msg, sock_client);
                        break;
                    case 3:
                        demander_liste_billets(received_msg,sock_client);
                        break;
                    case 4:
                        printf("User wants to join fil %d\n", ntohs(received_msg->numfil));
                        add_subscription_to_fil(received_msg, sock_client);
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

    // Initialise the fils
    fils = malloc(sizeof(fil *) * MAX_FIL);
    for (int i = 0; i < MAX_FIL; i++) {
       fils[i] = malloc(sizeof(fil));
       fils[i]->fil_number = i;
       fils[i]->head = malloc(sizeof(message_node));
       fils[i]->last_multicasted_message = malloc(sizeof(message_node));
       fils[i]->subscribed = 0;
       fils[i]->addrmult = malloc(sizeof(char) * 16);
   }

    // Start the notification thread
    pthread_t notification_thread;
    pthread_create(&notification_thread, NULL, send_notifications, (void *)fils);

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
    pthread_cancel(notification_thread);

    //*** fermeture socket serveur ***
    printf("Closing sock\n");
    close(sock);

    return 0;
}
