#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv){
	printf("Usage: %s <server ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n", argv[0]);
	exit(EXIT_FAILURE);
}

void verificar_argumentos(int argc, char **argv){
	if(argc < 8){
		printf("Error: Invalid number of arguments\n");
	}

	else if(strcmp(argv[3], "-type") != 0){
		printf("Error: Expected '-type' argument\n");
	}

	else if(strcmp(argv[4], "temperature") != 0 && strcmp(argv[4], "humidity") != 0 && strcmp(argv[4], "air_quality") != 0){
		printf("Error: Invalid sensor type\n");
	}

	else if(strcmp(argv[5], "-coords") != 0){
		printf("Error: Expected '-coords' argument\n");
	}

	else if(atoi(argv[6]) < 0 || atoi(argv[6]) > 9 || atoi(argv[7]) < 0 || atoi(argv[7]) > 9){
		printf("Error: Coordinates must be in the range 0-9\n");
	}
	usage(argc, argv);
}


#define BUFSZ 1024

int main(int argc, char **argv) {

	verificar_argumentos(argc, argv);

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
	printf("mensagem> ");
	fgets(buf, BUFSZ-1, stdin);
	size_t count = send(s, buf, strlen(buf)+1, 0);
	if (count != strlen(buf)+1) {
		logexit("send");
	}

	memset(buf, 0, BUFSZ);
	unsigned total = 0;
	while(1) {
		count = recv(s, buf + total, BUFSZ - total, 0);
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