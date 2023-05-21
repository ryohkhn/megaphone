#ifndef MEGAPHONE_CLIENT_H

#include "utilities.h"
#include <sys/stat.h>

#define directory_for_files "downloaded_files"
#define PORT 7777
#define LOCAL_ADDR "::1"

int clientfd;
int user_id;

// Adresse et port du serveur
char *server_addr;
int server_port;

// Liste des ports disponibles
int * available_ports;

// Mutex pour la gestion des ports
pthread_mutex_t port_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MEGAPHONE_CLIENT_H

#endif //MEGAPHONE_CLIENT_H
