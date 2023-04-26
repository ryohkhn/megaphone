#ifndef MEGAPHONE_SERVER_H

#include "utilities.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>


uint16_t id_dernier_client = 0;

list_client * clients;

// Liste des listes chainees pour les messages postes dans un fil
fil **fils;

int running = 1;
void signal_handler(int signal, siginfo_t *siginfo, void *context) {
    running = 0;
}

#define MEGAPHONE_SERVER_H

#endif //MEGAPHONE_SERVER_H
