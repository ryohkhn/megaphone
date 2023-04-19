#include "utilities.h"

void testMalloc(void *ptr){
    if(ptr==NULL){
        perror("Erreur de malloc() ou realloc().\n");
        exit(1);
    }
}

void print_8bits(uint8_t n){
    for(int i=0; i<=7; i++){
        uint8_t mask=1<<i;
        uint8_t bit=(n & mask)>>i;

        printf("%u",bit);
    }
    printf(" = %c\n",n);
}

void print_bits(uint16_t n){
    for(int i=0; i<=15; i++){
        uint16_t mask=1<<i;
        uint16_t bit=(n & mask)>>i;

        printf("%u",bit);
    }
    printf("\n");
}

entete *create_entete(uint8_t codereq,uint16_t id){
    entete* entete=malloc(sizeof(struct entete));
    entete->val=id;
    entete->val=entete->val<<5;
    entete->val=entete->val | codereq;
    entete->val=htons(entete->val);

    return entete;
}

void print_inscription_bits(inscription *msg){
    uint8_t *bytes=(uint8_t *) msg;
    for(size_t i=0; i<sizeof(inscription); i++){
        for(int j=7; j>=0; j--){
            printf("%d",(bytes[i]>>j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}