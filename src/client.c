#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


// Estrutura de comunicação entre cliente e o servidor.
struct sensor_message {
	char type[12];
	int coords[2];
	float measurement;
};


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
	}
	else if(strcmp(argv[4], "humidity") == 0){
		i = 1;
	}
	else if(strcmp(argv[4], "air_quality") == 0){
		i = 2;
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


#define BUFSZ 1024



int main(int argc, char **argv) {

	verificar_argumentos(argc, argv);
	// Mensagem de comunicação
	struct sensor_message message;
	sensor_message(argv, &message);

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

	printf("connected to %s\n", addrstr);

	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
	size_t count = send(s, &message, sizeof(message), 0);
	if (count != sizeof(message)){
		logexit("send");
	}

	memset(buf, 0, BUFSZ);
	unsigned total = 0;
	while(1) {
		count = recv(s, &message, sizeof(message), MSG_WAITALL);
		if (count != sizeof(message)) {
            logexit("recv");
        }
		if (count == 0) {
			// Connection terminated.
			break;
		}
		total += count;
	}
	close(s);

	printf("received %u bytes\n", total);
	puts(buf);

	exit(EXIT_SUCCESS);
}