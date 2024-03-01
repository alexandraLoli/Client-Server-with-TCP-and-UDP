#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

using namespace std;

int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char buffer[BUFLEN];

    if (argc < 4) {
        fprintf(stderr, "Usage: %s id_client server_address server_port\n", argv[0]);
	    exit(0);
    }

    // Obtinere port dorit
    int portnumber = atoi(argv[3]);
    DIE(portnumber == 0, "Error obtaining port number");

    
    // Setare informatii server
    struct sockaddr_in server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // pentru adrese IPv4
    server_addr.sin_port = htons(portnumber);
    
    int i;
    i = inet_aton(argv[2], &server_addr.sin_addr);
    DIE(i == 0, "Error inet_aton");

    // Structuri pentru gestionarea file descriptorilor
    fd_set read_fds; // multimea folosita in select()
    fd_set tmp_fds;  // multimea folosita temporar

    // Se golesc cele doua multimi
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    // Deschidere socket TCP
    int socketTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socketTCP < 0, "Error opening socket for TCP");

    // Reducere trafic pe retea
    int flag = 1;
	int ret = setsockopt(socketTCP, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	DIE(ret < 0, "Error disable neagle");

    // Conectare client la server
    int c;
    c = connect(socketTCP, (struct sockaddr*) &server_addr, sizeof(server_addr));
    DIE(c < 0, "Error connection");

    // Adaugare file descriptori
    FD_SET(socketTCP, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    int fdmax = socketTCP;


    // Obtinere ID client
    char id_client[10];
    memset(id_client, 0, sizeof(id_client));
    strcpy(id_client, argv[1]);
    DIE(strlen(id_client) > 10, "Error length id client");

    // Trimitere id la server
    int s;
    s = send(socketTCP, id_client, strlen(id_client) + 1, 0);
    DIE(s < 0, "Error sendind client ID");

    while (1) {
        tmp_fds = read_fds;

        // Verificarea conexiunii active
        int s;
        s = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(s < 0, "Error select");
        // Primire mesaj de la tastatura
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {

            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);

            if (!strncmp(buffer, "exit", 4)) {
                int n;
                n = send(socketTCP, (char*)"exit\n", strlen("exit\n"), 0);
				DIE(n < 0, "Error exit");
                close(socketTCP);
                exit(0);
            }
            else {
                int error;

                char sendDataToServer[BUFLEN];
                memset(sendDataToServer, 0, BUFLEN);

                strcpy(sendDataToServer, id_client);
                strcat(sendDataToServer, " ");
                strcat(sendDataToServer, buffer);
            
                if(strstr(sendDataToServer, "unsubscribe")) {

                    int r =  send(socketTCP, sendDataToServer, sizeof(sendDataToServer), 0);
                    DIE(r < 0, "Error unsubscribe");
                    printf("Unsubscribed from topic.\n");
                }
                else if (strstr(sendDataToServer, "subscribe")) {
                    
                    int r =  send(socketTCP, sendDataToServer, sizeof(sendDataToServer), 0);
                    DIE(r < 0, "Error subscribe");
                    printf("Subscribed to topic.\n");
                }
            } 
        }
        // Primire mesaj de la server
        else if (FD_ISSET(socketTCP, &tmp_fds)) {

            char message[BUFLEN];
            memset(message, 0, BUFLEN);

            int r;
            r = recv(socketTCP, message, BUFLEN, 0);
            DIE(r < 0, "Error receiving message");

            if (strstr(message, "exit") || r == 0) {
                exit(0);
            }
            else cout << message << endl;
        }
    
    }

    // Inchidere conexiune
    close(socketTCP);

    return 0;
}