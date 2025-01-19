#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>

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
	float distancia = sqrtf((dx * dx) +(dy * dy));
	return distancia;
}


float new_measure(struct sensor_message *message_init, struct sensor_message *message){
	
	float d = distancia_sensor(message_init, message);
	
	float diferenca = message->measurement - message_init->measurement;
	
	float ajuste = 0.1 * (1.0 / (d + 1.0));

	float nova_medida = message_init->measurement + (ajuste * diferenca);
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
	int tamanho;                 // Quantidade de elementos na lista
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

		// Verifica se o sensor já está inscrito na lista
		while(atual != NULL){
			if(atual->msg.coords[0] == message.coords[0] && atual->msg.coords[1] == message.coords[1]){
				return;
			}
			atual = atual->next;
		}

		atual = lista->head;

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


void remover_sensor(struct Lista* lista, struct sensor_message* message){

	// Verifica se a lista está vazia
	if(lista->head == NULL){
		return;
	}

	struct Node* atual = lista->head;

	// Varre a lista procurando o nó a ser removido
	while(atual != NULL){
		if(atual->msg.coords[0] == message->coords[0] && atual->msg.coords[1] == message->coords[1]){

			// Nó é o único da lista
			if(atual->prev == NULL && atual->next == NULL){
				lista->head = NULL;
				lista->tail = NULL;
			}

			// Nó é o primeiro da lista
			else if(atual->prev == NULL){
				lista->head = atual->next;
				lista->head->prev = NULL;
			}

			// Nó é o último da lista
			else if(atual->next == NULL){
				lista->tail = atual->prev;
				lista->tail->next = NULL;
			}

			// Nó está no meio da lista
			else{

				atual->prev->next = atual->next;
				atual->next->prev = atual->prev;
			}

			// Libera a memória
			free(atual);
			lista->tamanho--;
			return;
		}
		atual = atual->next;
	}
}

// Função para exibir a lista
void exibir_lista(struct Lista* lista){
	struct Node* atual = lista->head;

	while(atual != NULL){
		printf("Distancia (%d, %d): %.2f\n",atual->msg.coords[0], atual->msg.coords[1], atual->distancia);
		atual = atual->next;
	}
	printf("\n");
}


float ajustar_intervalo(float medida, struct sensor_message* message){
	// Ajustando intervalos
	float min[3] = {20.0, 10.0, 15.0};
	float max[3] = {40.0, 90.0, 30.0};
	
	
	char* type_sensor[3] = {"temperature", "humidity", "air_quality"};

	for(int i = 0; i < 3; i++){
		if(strcmp(message->type, type_sensor[i]) == 0){
			if(medida > max[i]){
				medida = max[i];
			}
			
			if(medida < min[i]){
				medida = min[i];
			}
		}
	}
	
	return medida;
}


void process_message(struct Lista* lista, struct sensor_message* message, struct sensor_message* message_init){

	float nova_medida = new_measure(message_init, message);
	
	
	nova_medida = ajustar_intervalo(nova_medida, message);
	float correction = nova_medida - message_init->measurement;


	struct Node* atual = lista->head;

	int i = 0;

	while(atual != NULL && i < 3){
		if((atual->msg.coords[0] == message->coords[0] && atual->msg.coords[1] == message->coords[1])){
			imprimir_message(message);
			printf("action:  correction  of %.4f\n", correction);
			printf("\n");
			message_init->measurement = nova_medida;
			return;
		}
		atual = atual->next;
		i++;
	}

	imprimir_message(message);
	printf("action: not  neighbor\n");
	printf("\n");
	

}


// Função para liberar memoria da lista
void liberar_memoria_lista(struct Lista* lista){

	struct Node* atual = lista->head;
	struct Node* proximo;

	while(atual != NULL){

		proximo = atual->next;
		free(atual);
		atual = proximo;
	}

	free(lista);
}


// Função para tratar interrupt keyboard
void handle_interrupt(int sig){
	exit(0);
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
	struct sensor_message message;
	struct sensor_message message_init;
	
	sensor_message(argv, &message);
	message_init = message;

	// Criando lista para armazenar os sensores por proximidade
	struct Lista* lista = criar_lista();
	float distancia = 0;

	// Configurando o sinal CTRL + C
	struct sigaction sa;
	sa.sa_handler = handle_interrupt;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	
	if(sigaction(SIGINT, &sa, NULL) == -1){
		perror("Erro SIGNIT");
		exit(EXIT_FAILURE);
	}

	// Envia mensagem para inscrever no tópico
	size_t count = send(s, &message, sizeof(message), 0);
	if (count != sizeof(message)){
			logexit("send");
	}


	message.measurement = message_init.measurement;

	// Início do temporizador
	clock_t start_time = clock();
    double elapsed_time;

	while(1) {
		
		elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;

		if(elapsed_time >= break_message){
			count = send(s, &message, sizeof(message), 0);
			if (count != sizeof(message)){
				logexit("send");
			}
			start_time = clock(); // reinicia o temporizador
		}

		else {

			count = recv(s, &message, sizeof(message), MSG_DONTWAIT);
	
			if(count == 1 || count == 0){
				close(s);
				exit(EXIT_SUCCESS);
			}
			
			if(count != -1){
				// Testa condição em que os sensores estão na mesma posição
				if(message.coords[0] == message_init.coords[0] && message.coords[1] == message_init.coords[1]){

					imprimir_message(&message);
					message.measurement = message_init.measurement;
					printf("action: same location \n");
					printf("\n");

				}
				// Sensor removed tem erro
				else if(message.measurement == -1){

					imprimir_message(&message);
					remover_sensor(lista, &message);
					printf("action: removed \n");
					printf("\n");
					message.measurement = message_init.measurement;
					message.coords[0] = message_init.coords[0];
					message.coords[1] = message_init.coords[1];	

				}

				else{
					// Cálculo da distância entre os sensores
					distancia = distancia_sensor(&message_init, &message);
					// Inserir elementos na lista
					inserir_ordenado(lista, message, distancia);
					// Processar tipo de messagem
					process_message(lista, &message, &message_init);

					message.measurement = message_init.measurement;
					message.coords[0] = message_init.coords[0];
					message.coords[1] = message_init.coords[1];	
				}
			}	
		}
	}
	liberar_memoria_lista(lista);
	close(s);


	exit(EXIT_SUCCESS);
}