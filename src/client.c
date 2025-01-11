#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int break_message = 0; // Intervalo entre as mensagens

// Mensagem padrão para ser exibida após cada mensagem de erro.
void usage(int argc, char **argv){
	printf("Usage: %s <server ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
	exit(EXIT_FAILURE);
}


// Função para tratar erros de inicialização.
void verificar_argumentos(int argc, char **argv){
	if(argc < 8){
		printf("Error: Invalid number of arguments\n");
		usage(argc, argv);
	}

	else if(strcmp(argv[3], "-type") != 0){
		printf("Error: Expected '-type' argument\n");
		usage(argc, argv);
	}

	else if(strcmp(argv[4], "temperature") != 0 && strcmp(argv[4], "humidity") != 0 && strcmp(argv[4], "air_quality") != 0){
		printf("Error: Invalid sensor type\n");
		usage(argc, argv);
	}

	else if(strcmp(argv[5], "-coords") != 0){
		printf("Error: Expected '-coords' argument\n");
		usage(argc, argv);
	}

	else if(atoi(argv[6]) < 0 || atoi(argv[6]) > 9 || atoi(argv[7]) < 0 || atoi(argv[7]) > 9){
		printf("Error: Coordinates must be in the range 0-9\n");
		usage(argc, argv);
	}
	
}


// Função para gerar medições aleatórias.
float medicao_aleatoria(char **argv){
	float min[3] = {20.0, 10.0, 15.0};
	float max[3] = {40.0, 90.0, 30.0};
	int i = 0;
	
	if(strcmp(argv[4], "temperature") == 0){
		i = 0;
		break_message = 5;
	}
	else if(strcmp(argv[4], "humidity") == 0){
		i = 1;
		break_message = 7;
	}
	else if(strcmp(argv[4], "air_quality") == 0){
		i = 2;
		break_message = 10;
	}


	srand(time(NULL)); // Inicializa a semente
	float real = (float)rand() / RAND_MAX; // Gera um número real entre 0.0 e 1.0
	float random_real_range = min[i] + (real * (max[i] - min[i])); // Ajusta para o intervalo [min, max]

	return random_real_range;
}


// Função para implementar a mensagem de comunicação.
void sensor_message(char **argv, struct sensor_message *message){

	snprintf(message->type, sizeof(message->type), "%s", argv[4]);
	message->coords[0] = atoi(argv[6]);
	message->coords[1] = atoi(argv[7]);
	message->measurement = medicao_aleatoria(argv);
}


float distancia_sensor(struct sensor_message *message_init, struct sensor_message *message){

	float dx = message_init->coords[0] - message->coords[0];
	float dy = message_init->coords[1] - message->coords[1];
	float distancia = sqrt((dx * dx) +(dy * dy));
	return distancia;
}


float new_measure(struct sensor_message *message_init, struct sensor_message *message){

	float d = distancia_sensor(&message_init, &message);
	float diferenca = message->measurement - message_init->measurement;
	float ajuste = 0.1 + (1 / (d + 1));
	float nova_medida = message->measurement + (ajuste * diferenca);
	return nova_medida;
	
}


struct Node{
	struct sensor_message msg;   // Identificador do sensor
	float distancia;			 // Distancia do sensor
	struct Node* next;           // Ponteiro para o próximo nó
	struct Node* prev;           // Ponteiro para o nó anterior
};


struct Lista{
	struct Node* head;           // Ponteiro para o início da lista
	struct Node* tail;           // Ponteiro para o final da lista
	int tamanho                  // Quantidade de elementos na lista
};


// Função para criar a lista dinamicamente
struct Lista* criar_lista(){
	struct Lista* lista = (struct Lista*)malloc(sizeof(struct Lista));
	lista->head = NULL;
	lista->tail = NULL;
	lista->tamanho = 0;
	return lista;
}


struct Node* criar_node(struct sensor_message message, float distancia){
	struct Node* novo = (struct Node*)malloc(sizeof(struct Node));
	novo->msg = message;
	novo->distancia = distancia;
	novo->next = NULL;
	novo->prev = NULL;
	return novo;
}

void inserir_ordenado(struct Lista* lista, struct sensor_message message, float distancia){
	struct Node* novo = criar_node(message, distancia);

	// Caso a lista esteja vazia
	if(lista->head == NULL){
		lista->head = novo;
		lista->tail = novo;
	}
	else{
		struct Node* atual = lista->head;

		// Encontrar a  posição correta para inserir
		while(atual != NULL && atual->distancia < distancia){
			atual = atual->next;
		}

		// Insere no início
		if(atual == lista->head){
			novo->next = lista->head;
			lista->head->prev = novo;
			lista->head = novo;
		}

		// Insere no final
		else if(atual == NULL){
			novo->prev = lista->tail;
			lista->tail->next = novo;
			lista->tail = novo;
		}

		// Insere no meio
		else {
			novo->next = atual;
			novo->prev = atual->prev;
			atual->prev->next = novo;
			atual->prev = novo;
		}
	}

	lista->tamanho++; // Atualiza tamanho da lista
}

#define BUFSZ 1024



int main(int argc, char **argv) {

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	//printf("connected to %s\n", addrstr);

	verificar_argumentos(argc, argv);

	// Mensagem de comunicação
	struct sensor_message message, message_init;
	
	sensor_message(argv, &message);
	message_init = message;

	while(1) {
		
		message_init.measurement = message.measurement;
		size_t count = send(s, &message, sizeof(message), 0);
		if (count != sizeof(message)){
			logexit("send");
		}
		count = recv(s, &message, sizeof(message), MSG_WAITALL);
		imprimir_message(&message);
		sleep(break_message);
		
	}
	close(s);


	exit(EXIT_SUCCESS);
}