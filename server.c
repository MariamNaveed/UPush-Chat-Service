#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#include "send_packet.h"

#define BUFFSIZE 255

void checkPerror(int i, char *msg)
{
    if (i == -1)
    {
        perror(msg);
        exit(1);
    }
}

struct client
{
    char *ip;
    int port;
    char *nick;
    time_t last_ack;
    struct client *next;
};

struct client *clients = NULL; // globaleLinkedList

void registerC(char *name, struct sockaddr_in *addr)
{
    struct client *theClient = malloc(sizeof(struct client));
    if (theClient == NULL)
    {
        fprintf(stderr, "Cannot allocate initial memory for data\n");
        exit(1);
    }
    theClient->ip = inet_ntoa(addr->sin_addr);
    theClient->nick = strdup(name);
    if (theClient->nick == NULL)
    {
        perror("strdup");
        exit(1);
    }
    theClient->port = addr->sin_port;
    theClient->next = clients;
    theClient->last_ack = time(NULL);
    clients = theClient;

    struct client *peker = NULL;
    peker = clients;

    for (peker = clients; peker != NULL; peker = peker->next)
    {
        printf("port: %d\n", peker->port);
        printf("IP: %s\n", peker->ip);
    }

    
}

struct client *look_upC(char *name, struct client *head)
{
    struct client *current = head;

    while (current != NULL)
    {
        if (strcmp(current->nick, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void deleteClient(char *name, struct client *head)
{
    struct client *current = head, *prev;

    if (current == NULL)
    {
        return;
    }
    if (current != NULL && current->nick == name)
    {
        head = current->next;
        return;
    }

    while (current != NULL && current->nick != name)
    {
        prev = current;
        current = current->next;
    }

    prev->next = current->next;

    free(current);
}


void free_linkedList(struct client *head){
    struct client *tmp = head;
    while(tmp != NULL){
        tmp = tmp->next;
        free(tmp->nick);
        free(tmp);
    }
}

int main(int argc, char const *argv[])
{
    srand48(time(NULL));

    if (argc < 3 || argc > 3)
    {
        printf("Not valid input. Try: <Port> <Tapsannsynlighet>.\n");
        exit(EXIT_FAILURE);
    }

    float probability_command = atof(argv[2]); 
    float probability = probability_command / 100;

    set_loss_probability(probability);
    int fd, rc, wc;
    fd_set fds;
    struct sockaddr_in my_addr, src_addr;
    char buffer[BUFFSIZE]; 
    socklen_t addr_len;
    time_t curr_time;


    fd = socket(AF_INET, SOCK_DGRAM, 0); 
    checkPerror(fd, "socket");

    uint16_t port = atoi(argv[1]); 

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);       
    my_addr.sin_addr.s_addr = INADDR_ANY; 

    rc = bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in)); 
    checkPerror(rc, "bind");

    

    while (1)
    {

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        wc = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
        checkPerror(rc, "select");

        addr_len = sizeof(struct sockaddr_in);
        rc = recvfrom(fd, buffer, BUFFSIZE - 1, 0, (struct sockaddr *)&src_addr, &addr_len);
        checkPerror(rc, "recvfrom");
        buffer[rc] = 0; 
        printf("%s\n", buffer);

        char space[] = " \n";
        char *result[4];
        int i = 0;
        result[i] = strtok(buffer, space);

        while (result[i] != NULL)
        {
            result[++i] = strtok(NULL, space);
        }

        char *commando = result[2];
        char *name = result[3];
        int len = strlen(name);
        

        if (len > 20)
        {
            char buf[] = "Name is too long, write a name less than 20 char";
            wc = send_packet(fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&src_addr, addr_len);
            checkPerror(wc, "send1");
        }

        char *ack = "ACK";

        struct client *check_reg = look_upC(name, clients);

        if (strcmp(commando, "REG") == 0)
        {
            if (check_reg != NULL)
            {

                check_reg->ip = inet_ntoa(src_addr.sin_addr);
                check_reg->nick = name;
                check_reg->port = src_addr.sin_port;
            }
            else
            {
                registerC(name, &src_addr);
                char buf1[50];
                char *ok = "OK";
                sprintf(buf1, "%s %s %s", ack, result[1], ok);
                wc = send_packet(fd, buf1, strlen(buf1) + 1, 0, (struct sockaddr *)&src_addr, addr_len);
                checkPerror(wc, "send2");
            }

        }
        else
        {
            if (strcmp(commando, "LOOKUP") == 0)
            {
                struct client *check = look_upC(name, clients);
                if (check != NULL)
                {
                    curr_time = time(NULL);
                    

                    if (curr_time - check->last_ack > 30)
                    {
                        deleteClient(check->nick, clients);
                        char msg[50];
                        sprintf(msg, "%s %s %s %s", "ACK", result[1], "NOT", "FOUND");
                        wc = send_packet(fd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&src_addr, addr_len);
                        checkPerror(wc, "send_msg");
                    }
                    else
                    {
                        char buf2[50];
                        char *nick = "NICK";
                        char *ip = "IP";
                        char *port = "PORT";
                        sprintf(buf2, "%s %s %s %s %s %s %s %d\n ", ack, result[1], nick, result[3], ip, inet_ntoa(src_addr.sin_addr), port, check->port);
                        wc = send_packet(fd, buf2, strlen(buf2) + 1, 0, (struct sockaddr *)&src_addr, addr_len);

                        checkPerror(wc, "send3");
                    }
                }
                else
                {
                    char buf3[50];
                    char *notFound = "NOT FOUND";
                    sprintf(buf3, "%s %s %s\n ", ack, result[1], notFound);
                    wc = send_packet(fd, buf3, strlen(buf3) + 1, 0, (struct sockaddr *)&src_addr, addr_len);
                    checkPerror(wc, "send4");
                }
            }
        }
    }
    free_linkedList(clients);
    close(fd);
    return EXIT_SUCCESS;
}