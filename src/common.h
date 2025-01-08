#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

// Estrutura de comunicação entre cliente e o servidor.
struct sensor_message {
	char type[12];
	int coords[2];
	float measurement;
};

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

void imprimir_message(struct sensor_message *message);
