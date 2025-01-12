#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024
#define MAX_CLIENTS 12 // Máximo de assinantes por tópico
#define NUM_TOPICS 3  // Número de tópicos

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};


void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}


struct Topic {
    char name[12];                   // Nome do tópico
    int subscriber_count;             // Número de assinantes
    int subscribers[MAX_CLIENTS];    // Lista de IDs dos assinantes (sockets)
    pthread_mutex_t lock;            // Mutex para evitar race condition a lista

};

struct Topic topic[NUM_TOPICS];

void inicialize_topics(){

    
    char *topicos[] = {
        "temperature",
        "humidity",
        "air_quality"
    };

    // Loop para inicializar os tópicos
    for(int i = 0; i < NUM_TOPICS; i++){
        strncpy(topic[i].name, topicos[i], sizeof(topic[i].name));
        topic[i].subscriber_count = 0;
        // Inicializa o mutex
        pthread_mutex_init(&topic[i].lock, NULL);
    }

}


void subscribe_to_topic(struct sensor_message *message, int csock){

    for(int i = 0; i < NUM_TOPICS; i++){
        if(strcmp(topic[i].name, message->type) == 0){
            pthread_mutex_lock(&topic[i].lock); // Bloqueia acesso ao tópico

            // Verifica se o cliente já está inscrito no tópico
            for(int j = 0; j < topic[i].subscriber_count; j++){
                if(topic[i].subscribers[j] == csock){
                    pthread_mutex_unlock(&topic[i].lock);
                    return;
                }
            }
            // Adiciona assinante ao topíco
            if(topic[i].subscriber_count < MAX_CLIENTS){
                topic[i].subscribers[topic[i].subscriber_count] = csock;
                topic[i].subscriber_count++;
            }
            pthread_mutex_unlock(&topic[i].lock); // Libera o mutex
            return;

        }
    }
}


void send_to_subscribe(struct sensor_message message, int csock, ssize_t count){

    for(int i = 0; i < NUM_TOPICS; i++){
        if(strcmp(topic[i].name, message.type) == 0){
            pthread_mutex_lock(&topic[i].lock);

            // Percorre todos os assinantes do tópico
            for(int j = 0; j < topic[i].subscriber_count; j++){
                // Envia mensagem para os assinantes do tópico
                count = send(topic[i].subscribers[j], &message, sizeof(message), 0);
                if (count != sizeof(message)) {
                    logexit("send");
                }
            }
        }
        pthread_mutex_unlock(&topic[i].lock);
    }
}


void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);


    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    struct sensor_message message;

    sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
    
    int csock = cdata->csock;
    
    while(1){

        ssize_t count = recv(csock, &message, sizeof(message), MSG_WAITALL);
        
        imprimir_message(&message);
        printf("\n");
        // Assina o topíco
        subscribe_to_topic(&message, csock);
        send_to_subscribe(message, csock, count);

    }
    close(cdata->csock);

    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < NUM_TOPICS) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    
    inicialize_topics();

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

	struct client_data *cdata = malloc(sizeof(*cdata));
	if (!cdata) {
		logexit("malloc");
	}
	cdata->csock = csock;
	memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
