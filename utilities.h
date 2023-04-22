#ifndef MEGAPHONE_UTILITIES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

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

void testMalloc(void *ptr);

void print_8bits(uint8_t n);

void print_bits(uint16_t n);

void print_inscription_bits(inscription *msg);

entete *create_entete(uint8_t codereq,uint16_t id);

char* client_message_to_string(client_message *msg);

client_message* string_to_client_message(const char *str);

#define MEGAPHONE_UTILITIES_H

#endif //MEGAPHONE_UTILITIES_H
