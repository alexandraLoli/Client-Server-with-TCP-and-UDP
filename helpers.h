#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>

using namespace std;

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1501	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	100	// numarul maxim de clienti in asteptare

// Structuri

typedef struct {
	int socket;
	bool connection_status;
	queue<string> delayed_messeges;
} Client;

typedef struct {
	int SF;
	char id[10];
} Subscriber;



#endif
