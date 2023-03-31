#ifndef MEGAPHONE_SERVER_H

#include "utilities.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

typedef struct entete{
    uint16_t val;
} entete;

typedef struct res_inscription{
    uint16_t id;
} res_inscription;

typedef struct inscription_message{
    entete entete;
    char pseudo[10];
} inscription;

typedef struct client_message{
    entete entete;
    uint16_t numfil;
    uint16_t nb;
    uint8_t *data;
} client_message;

typedef struct server_message{
    entete entete;
    uint16_t numfil;
    uint16_t nb;
} server_message;

typedef struct list_client{
    unsigned long id;
    char * pseudo;
    struct list_client * suivant;
}list_client;

uint16_t id_dernier_client = 0;

list_client * clients;
// possibilité d'optimisation en utilisant un tableau au lieu d'une liste.

#define MEGAPHONE_SERVER_H

#endif //MEGAPHONE_SERVER_H
