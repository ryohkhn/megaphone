#ifndef MEGAPHONE_SERVER_H

#include "utilities.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

// Maximum number of fils possible (2^16 - 1)
#define MAX_FIL 65536
pthread_mutex_t fil_mutex[MAX_FIL];
void init_fil_mutex() {
    for (int i = 0; i < MAX_FIL; i++) {
        pthread_mutex_init(&fil_mutex[i], NULL);
    }
}

// Maximum number of clients possible (2^11 - 1)
#define MAX_ID 2047
uint16_t id_dernier_client = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
list_client * clients;

// Liste des listes chainees pour les messages postes dans un fil
fil *fils;
uint16_t fils_size = 0;
uint16_t fils_capacity = 1;

int running = 1;
void signal_handler(int signal, siginfo_t *siginfo, void *context) {
    running = 0;
}

#define MEGAPHONE_SERVER_H

#endif //MEGAPHONE_SERVER_H
