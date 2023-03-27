#ifndef MEGAPHONE_CLIENT_H

#include "utilities.h"

int clientfd;

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


#define MEGAPHONE_CLIENT_H

#endif //MEGAPHONE_CLIENT_H
