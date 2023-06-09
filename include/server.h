#ifndef MEGAPHONE_SERVER_H

#include "utilities.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

// Maximum number of fils possible (2^16 - 1)
#define MAX_FIL 65536
#define PORT 7777
#define MULTICAST_BASE_ADDRESS "ff02::1:1"
#define directory_for_files "fichiers_fil"

// Maximum number of clients possible (2^11 - 1)
#define MAX_ID 2047
uint16_t id_dernier_client = 0;
list_client * clients;

// Liste des listes chainees pour les messages postes dans un fil
fil *fils;
uint16_t fils_size = 0;
uint16_t fils_capacity = 1;

int running = 1;
void signal_handler(int signal, siginfo_t *siginfo, void *context) {
    running = 0;
}

// Adresse et port du serveur
int server_port;

// Liste des ports disponibles
int * available_ports;

// Mutexs for each thread
pthread_mutex_t fil_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex pour la gestion des ports
pthread_mutex_t port_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex pour la gestion de la liste de clients
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex pour la taille de la liste de threads
pthread_mutex_t thread_size_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_args {
    int *sock_client;
    char client_ip[INET6_ADDRSTRLEN];
} thread_args;

#define MEGAPHONE_SERVER_H

#endif //MEGAPHONE_SERVER_H
