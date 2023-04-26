#ifndef MEGAPHONE_UTILITIES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define NOTIFICATION_INTERVAL 5
#define MULTICAST_PORT 49152
#define MAX_FIL 65535

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

typedef struct server_billet{
    uint16_t numfil;
    uint8_t *origine;
    uint8_t *pseudo;
    uint8_t *data;
} server_billet;

typedef struct notification{
  entete entete;
  uint16_t numfil;
  uint8_t *pseudo;
  uint8_t *data;
} notification;

typedef struct server_message{
    entete entete;
    uint16_t numfil;
    uint16_t nb;
} server_message;

typedef struct server_subscription_message{
  entete entete;
  uint16_t numfil;
  uint16_t nb;
  uint8_t *addrmult;
} server_subscription_message;

// List of fils
typedef struct message_node {
    client_message *msg;
    struct message_node *next;
} message_node;


// Specific fil as a list of messages
typedef struct fil {
    uint16_t fil_number;
    message_node *head;
    message_node *last_multicasted_message;
    char* addrmult;
    int subscribed;
} fil;

typedef struct list_client{
    unsigned long id;
    char * pseudo;
    struct list_client * suivant;
}list_client;


list_client * clients;
// possibilité d'optimisation en utilisant un tableau au lieu d'une liste.

void testMalloc(void *ptr);

void print_8bits(uint8_t n);

void print_bits(uint16_t n);

void print_inscription_bits(inscription *msg);

entete *create_entete(uint8_t codereq,uint16_t id);

char* pseudo_from_id(int id);

char* client_message_to_string(client_message *msg);

char* server_subscription_message_to_string(server_subscription_message *msg);

client_message* string_to_client_message(const char *str);

server_billet* string_to_server_billet(const char *buffer);

server_message *string_to_server_message(const char *buffer);

server_subscription_message *string_to_server_subscription_message(const char *buffer);

char *client_message_to_notification(client_message *msg);

notification *string_to_notification(const char* buffer);

uint16_t get_id_entete(uint16_t ent);

uint16_t chars_to_uint16(char a,char b);

void add_message_to_fil(fil **fils, client_message *msg, uint16_t fil_number);

char** retrieve_messages_from_fil(fil **fils, uint16_t fil_number);


#define MEGAPHONE_UTILITIES_H

#endif //MEGAPHONE_UTILITIES_H
