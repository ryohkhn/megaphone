#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

typedef struct entete {
    uint16_t codereq : 5 ;
    uint16_t id : 11;
} entete;

typedef struct inscription_message {
    entete entete;
    char pseudo[10];
} inscription;

struct client_message {
    entete entete;
    uint16_t numfil;
    uint16_t nb;
    uint16_t datalen;
    char data[];
};

void testMalloc(void *ptr){
  if(ptr == NULL){
    perror("Erreur de malloc() ou realloc().\n");
    exit(1);
  }
}

entete* create_entete(uint8_t codereq, uint16_t id) {
  //TODO make sure it's big endian...
    entete* entete = malloc(sizeof(entete));
    entete->codereq = (codereq & 0x1F) << 3; // keep only the 5 least significant bits of codereq
    entete->id = htons(id);

    return entete;
}


inscription *create_inscription(char pseudo[]){
  if(strlen(pseudo) != 10){
    char new_pseudo[10];
    strncpy(new_pseudo, pseudo, 10);
    for (size_t i = strlen(new_pseudo); i < 10; i++) {
      new_pseudo[i] = '#';
    }
    pseudo = new_pseudo;
  }

  inscription *inscription_message = malloc(sizeof(inscription));
  testMalloc(inscription_message);

  entete *entete = create_entete(1,0);
  testMalloc(entete);

  memcpy(&(inscription_message->entete), entete, sizeof(*entete));
  strncpy(inscription_message->pseudo, pseudo, 10);

  free(entete);
  return inscription_message;
}

void print_inscription_bits(inscription* msg) {
    uint8_t* bytes = (uint8_t*) msg;
    for (size_t i = 0; i < sizeof(inscription); i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (bytes[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}



void test(){
  struct entete entete;
  printf("%ld \n" , sizeof(entete)); // 2 octets

  char pseudo[] = "test";
  inscription *i = create_inscription(pseudo);

  printf("%d %d %s \n", i->entete.codereq , i->entete.id , i->pseudo); // 1 0 test######

  print_inscription_bits(i);
}

int main(void){
  test();
  return 0;
}
