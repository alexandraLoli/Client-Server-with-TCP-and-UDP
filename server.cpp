#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <iostream>
#include <queue>
#include <math.h>
#include <unordered_set>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fstream>
#include "helpers.h"

using namespace std;

int main(int argc, char *argv[]) {    

    // Hashtable de clienti (id client -> client)
    unordered_map<string, Client> clients;

    // Hashtable de abonati (topic -> abonari la topic)
    unordered_map<string, vector<Subscriber>> subscription_list;
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char buffer[BUFLEN];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s server_port\n", argv[0]);
	    exit(0);
    }

    // Obtinere port dorit
    int portnumber = atoi(argv[1]);
    DIE(portnumber == 0, "Error obtaining port number");

    // Setare structurii sockaddr_in pentru ascultarea portului respectiv
    struct sockaddr_in server_addr;
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // pentru adrese IPv4
    server_addr.sin_port = htons(portnumber);
    server_addr.sin_addr.s_addr = INADDR_ANY; // se poate asculta pe orice interfata de retea disponibila

    // Structuri pentru gestionarea file descriptorilor
    fd_set read_fds; // multimea folosita in select()
    fd_set tmp_fds;  // multimea folosita temporar

    // Se golesc cele doua multimi
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    //  Deschidere socket pentru TCP
    int socketTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socketTCP < 0, "Error opening socket for TCP");

    // Reducere trafic pe retea
    int flag = 1;
	int ret = setsockopt(socketTCP, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	DIE(ret < 0, "Error disable neagle");

    // Deschidere socket pentru UDP
    int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(socketUDP < 0, "Error opening socket for UDP");

    // Setare socketi la adrese specifice
    int b;
    b = bind(socketTCP, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
    DIE(b < 0, "Error bind TCP");

    b = bind(socketUDP, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
    DIE(b < 0, "Error bind UDP");

    // Pregatire socket pentru conexiuni
    int l;
    l = listen(socketTCP, MAX_CLIENTS);
    DIE(l < 0, "Error listen");

    // Adaugare file descriptori
    FD_SET(socketTCP, &read_fds);
    FD_SET(socketUDP, &read_fds);
    FD_SET(0, &read_fds);

    // Setare FileDescriptor maxim intre conexiunile curente
    int fdmax = 0;
    if (socketTCP > socketUDP) {
        fdmax = socketTCP;
    } else {
        fdmax = socketUDP;
    }

    while(1) {

        tmp_fds = read_fds;
        struct sockaddr_in client_addr;
        socklen_t client_len;

        // Verificarea conexiunii active
        int s;
        s = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(s < 0, "Error select");

        // Scanarea datelor
        for(int con = 0; con <= fdmax; con++) {

            // Conexiunea curenta este activa
            if (FD_ISSET(con, &tmp_fds)) {

                // Mesaj de la socketul TCP
                if (con == socketTCP) {

                    // Acceptare cerere de conexiune
                    client_len = sizeof(client_addr);
                    int newSocket = accept(socketTCP, (struct sockaddr *) &client_addr, &client_len);
                    DIE(newSocket < 0, "Error accept");

                    // Reducere trafic pe retea
                    int flag = 1;
	                int ret = setsockopt(newSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	                DIE(ret < 0, "Error disable neagle");

                    // Primire ID de la client
                    memset(buffer, 0, BUFLEN);
                    int r;
                    r = recv(newSocket, buffer, sizeof(buffer), 0);
                    DIE(r < 0, "Error receiving ID");


                    char id_client[10];
                    memset(id_client, 0, sizeof(id_client));
                    strncpy(id_client, buffer, sizeof(id_client));

                    // Client nou
                    if (clients.find(id_client) == clients.end()) {

                        Client new_client;
                        new_client.connection_status = true;
                        new_client.socket = newSocket;
                        clients.insert(make_pair(id_client, new_client));
                    }

                    // Client existent
                    else {
                        // care nu era activ
                        if (!clients[id_client].connection_status) {

                            clients[id_client].connection_status = true;
                            clients[id_client].socket = newSocket;

                            // Trimitere mesaje
                            while (!clients[id_client].delayed_messeges.empty()) {

                                char message[1501];
                                memset(message, 0, 1501);
                                strcpy(message, clients[id_client].delayed_messeges.front().c_str());
                                int s;
                                s = send(newSocket, message, sizeof(message), 0);
                                DIE(s < 0, "Error sending message");
                                clients[id_client].delayed_messeges.pop(); 
                            }
                        }
                        else {
                            int n;
                            n = send(newSocket, (char *)"exit.\n", strlen("exit.\n"), 0);
                            DIE (n < 0, "Error send exit");
                            cout << "Client " << id_client << " already connected." << endl;
                            close(newSocket);
                            continue;
                        }
                    }

                    
                    FD_SET(newSocket, &read_fds);
                    if (newSocket > fdmax) {
                        fdmax = newSocket;
                    }

                    printf("New client %s connected from %s:%d\n", 
                    id_client, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                } 

                // Mesaj de la clientul UDP
                else if (con == socketUDP) {
                    
                    // Primire mesaj
                    client_len = sizeof(client_addr);
                    memset(buffer, 0, BUFLEN);
                    int r;
                    r = recvfrom(socketUDP, buffer, BUFLEN, 0, (struct sockaddr *) &server_addr, &client_len);
                    DIE(r < 0, "Error reveice");

                    uint16_t port = server_addr.sin_port;

                    char ip[20];
                    memset(ip, 0, 20);
                    memcpy(ip, inet_ntoa(server_addr.sin_addr), 20);
                
                    char topic[51];
                    memset(topic, 0, 50);
                    memcpy(topic, buffer, 50);

                    uint8_t data_type = buffer[50];

                    char content[BUFLEN];
                    memset(content, 0, BUFLEN);
                    strcat(content, topic);
                    strcat(content, " - ");

                    switch (data_type) {
                        case 0: {
                            strcat(content, "INT - ");
                            uint32_t value;
                            memset(&value, 0, sizeof(uint32_t));
                            memcpy(&value, buffer + 52, sizeof(uint32_t));
                            if (buffer[51] == 1) {
                                strcat(content, "-");
                                strcat(content, to_string(ntohl(value)).c_str());
                            }
                            else {
                                strcat(content, to_string(ntohl(value)).c_str());
                            }
                            break;
                        }
                        case 1: {
                            uint16_t value;
                            memset(&value, 0, sizeof(uint16_t));
                            memcpy(&value, buffer + 51, sizeof(uint16_t));
                            strcat(content, "SHORT_REAL - ");
                            strcat(content, to_string(1.0 * ntohs(value) / 100).c_str());
                            break;
                        }
                        case 2: {
                            strcat(content, "FLOAT - ");
                            uint32_t modvalue;
                            memset(&modvalue, 0, sizeof(uint32_t));
                            memcpy(&modvalue, buffer + 52, sizeof(uint32_t));
                            uint8_t expvalue;
                            memset(&expvalue, 0, sizeof(uint8_t));
                            memcpy(&expvalue, buffer + 52 + sizeof(uint32_t), sizeof(uint8_t));
                            float floatvalue = ntohl(modvalue);

                            while (expvalue) {
                                floatvalue = floatvalue / 10;
                                expvalue --;
                            }
                            
                            if (buffer[51] == 1) {
                                strcat(content, "-");
                                strcat(content, to_string(floatvalue).c_str());
                            }
                            else {
                                strcat(content, to_string(floatvalue).c_str());
                            }
                            break;
                        }
                        case 3: {
                            strcat(content, "STRING - ");
                            char data[BUFLEN];
                            memset(data, 0, BUFLEN);
                            memcpy(data, buffer + 51, BUFLEN);
                            strcat(content, data);
                            break;
                        }
                    }

                    vector<Subscriber> subscribers = subscription_list[topic];

                    for(auto subscriber : subscribers) {

                        // Daca clientul e conectat, ii trimitem mesajul
                        if (clients[subscriber.id].connection_status) {
                            int s;
                            s = send(clients[subscriber.id].socket, content, sizeof(content), 0);
                            DIE(s < 0, "Error sending message");
                        }
                        else {
                            // Clientul e deconectat, dar SF = 1
                            if (subscriber.SF) {
                                clients[subscriber.id].delayed_messeges.push(content);
                            }
                        }
	                }

                }

                // Mesaj de la STDIN
                else if (con == STDIN_FILENO) {
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN, stdin);

                    if(strstr(buffer, "exit")) {
                        for (auto client : clients) {
                            if (client.second.connection_status) {
                                close(client.second.socket);
                                cout << "Client " << client.first << " disconnected\n";
                            }
                        }
                        // Inchidere conexiuni
                        for (int i = 0; i <= fdmax; i++) {
                            if(FD_ISSET(i, &read_fds)) {
                                close(i);
                            }
                        }
                        exit(0);
                    }
                }

                //Mesaj de la client
                else {

                    // Primire mesaj de la client
                    memset(buffer, 0, BUFLEN);
                    
                    int receive = recv(con, buffer, sizeof(buffer), 0);
                    DIE(receive < 0, "Error receive message");

                    // Daca nu am mai primit nimic de la client sau comanda e "exit"
                    if (receive == 0 || strstr(buffer, "exit")) {

                        // Inchidere conexiune

                        char id[10];
                        memset(id, 0, 10);

                        for (auto client : clients) {
                            if (client.second.socket == con)
                                strcpy(id, client.first.c_str());
                                
                        }

                        printf("Client %s disconnected.\n", id);

                        clients[id].connection_status = false;
                        close(con);
                        FD_CLR(con, &read_fds);
                        continue;
                    }

                    char copybuf[BUFLEN];
                    memset(copybuf, 0, BUFLEN);
                    memcpy(copybuf, buffer, strlen(buffer));

                    char *id = strtok(buffer, " \n");
                    char *command = strtok(NULL, " \n");
                    char *topic = strtok(NULL, " \n");
                    
                    // Primire mesaj de dezabonare
                    if (strstr(copybuf, "unsubscribe")) {

                        vector<Subscriber> subscribers = subscription_list[topic];
                        for (int i = 0; i < subscribers.size(); i++) {
                            if (!strcmp(subscribers[i].id, id)) {
                                subscription_list[topic].erase(subscription_list[topic].begin() + i);
                                break;
                            }
                        }
                    }

                    // Primire mesaj de abonare
                    else if (strstr(command, "subscribe")) {

                        char *sf = strtok(NULL, "\n");
                        
                        // Creare abonat nou
                        Subscriber subscriber;
                        memset(subscriber.id, 0, sizeof(subscriber.id));
                        memcpy(subscriber.id, id, strlen(id));
                        if (sf[0] == '0')
                            subscriber.SF = 0;
                        else 
                            subscriber.SF = 1;

                        // Daca topicul nu exista in lista
                        if (subscription_list.find(topic) == subscription_list.end()) {
                            vector<Subscriber> new_subscribers;
                            new_subscribers.push_back(subscriber);
                            subscription_list.insert(make_pair(topic, new_subscribers));
                        }
                        else {
                            subscription_list[topic].push_back(subscriber);
                        }
                        
                    }
                }
            }
        }
    }

    // Inchidere conexiuni
    for (int i = 0; i <= fdmax; i++) {
        if(FD_ISSET(i, &read_fds)) {
            close(i);
        }
    }


    return 0;
}
