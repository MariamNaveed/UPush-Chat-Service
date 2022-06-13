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
#include <stdint.h>
#include <time.h>

#include "send_packet.h"

#define BUFFSIZE 1400

struct message
{
    char *text;
    char *nick;
    int id;
    struct message *next;
};

struct clientCache
{
    char *ip;
    time_t last_reg;
    int port;
    char *nick;
    struct clientCache *next;
    struct message *messageList;
};

void checkPerror(int i, char *msg)
{
    if (i == -1)
    {
        perror(msg);
        exit(1);
    }
}

struct clientCache *clientsCacheList = NULL; // globalLinkedList
struct message *messageList = NULL;          // head

void registerM(char *name, char *text, int id)
{

    struct message *theMessage = malloc(sizeof(struct message));

    struct message *last = NULL;
    last = messageList; // last = head

    if (theMessage == NULL)
    {
        fprintf(stderr, "Cannot allocate initial memory for data\n");
        exit(1);
    }
   
    theMessage->nick = strdup(name);
    if (theMessage->nick == NULL)
    {
        perror("strdup");
        exit(1);
    }
    theMessage->text = strdup(text);
    theMessage->id = id;


    theMessage->next = NULL;

    
    if (messageList == NULL)
    {
        messageList = theMessage;
        
    }
    else
    {
        while (last->next != NULL)
        {
            last = last->next;
        }
        
        last->next = theMessage;
        return;
    }


}

void registerC(char *name, struct sockaddr_in *addr)
{
    struct clientCache *theClient = malloc(sizeof(struct clientCache));
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
    int temp = addr->sin_port;
    theClient->port = ntohs(temp);
    theClient->next = clientsCacheList;
    theClient->last_reg = time(NULL);
    clientsCacheList = theClient;
}

struct clientCache *look_upC(char *name, struct clientCache *head)
{
    struct clientCache *current = head;

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


void get_string(char buf[], int size)
{
    char c;
    fgets(buf, size, stdin);

    
    if (buf[strlen(buf) - 1] == '\n')
    {
        buf[strlen(buf) - 1] = '\0';
    }
   
    else
        while ((c = getchar()) != '\n' && c != EOF)
            ;
}
//free nick-cache
void free_ClinkedList(struct clientCache *head){
    struct clientCache *tmp = head;
    while(tmp != NULL){
        tmp = tmp->next;
        free(tmp->nick);
        free(tmp);
    }
}

void free_MlinkedList(struct message *head){
    struct message *tmp = head;
    while(tmp != NULL){
        tmp = tmp->next;
        free(tmp->nick);
        free(tmp->text);
        free(tmp);
    }
}


int main(int argc, char const *argv[])
{
    srand48(time(NULL)); 

    if (argc < 6 || argc > 6)
    {
        printf("Not valid input. Try: <nick> <adresse> <port> <timeout> <tapssannsynlighet>.\n");
        exit(EXIT_FAILURE);
    }
    float probability_command = atof(argv[5]); 

    float probability = probability_command / 100;

    set_loss_probability(probability);

    int fd, wc, rc;
    fd_set fds;
    char buffer[BUFFSIZE];
    char msg_buffer1[BUFFSIZE];
    struct sockaddr_in dest_addr, clientaddr; 
    struct in_addr ip_addr, ip_addr_client;
    socklen_t addr_len;
    time_t last_reg, curr_time;
    struct timeval tv;

    fd = socket(AF_INET, SOCK_DGRAM, 0); 
    checkPerror(fd, "socket");

    wc = inet_pton(AF_INET, argv[2], &ip_addr);
    checkPerror(wc, "ip");
    if (!wc) 
    {
        fprintf(stderr, "Invalid IP address: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    uint16_t portServer = atoi(argv[3]);

    // server
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(portServer); 
    dest_addr.sin_addr = ip_addr;          
    addr_len = sizeof(struct sockaddr_in);

    char buf1[50];
    char *pkt = "PKT";
    char *sequencenb = "0";
    char *reg = "REG";
    sprintf(buf1, "%s %s %s %s", pkt, sequencenb, reg, argv[1]);

    wc = send_packet(fd, buf1, strlen(buf1) + 1, 0, (struct sockaddr *)&dest_addr, addr_len);

    int timeout = atoi(argv[4]);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    
    if (FD_ISSET(fd, &fds))
    {
        rc = recv(fd, buffer, BUFFSIZE - 1, 0);
        checkPerror(rc, "recvfrom");
        buffer[rc] = 0;                          
        printf("%s\n", buffer); 
    }
    else if (wc == 0)
    {

        printf("Melding ikke motatt\n");
        exit(1);

        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        FD_SET(fd, &fds);
    }

    printf("Welcome to UPush chat. Write QUIT to leave\n");

    char output[BUFFSIZE];
    last_reg = time(NULL);

    tv.tv_sec = 10;
    tv.tv_usec = 0;
    while (1)
    {
        fflush(NULL);
        char *portClient = NULL;

        char *tekst = NULL;
        int id_msg = 0;

        curr_time = time(NULL);
        if (curr_time - last_reg >= 10)
        {
            char buf1[50];
            char *pkt = "PKT";
            char *sequencenb = "0";
            char *reg = "REG";
            sprintf(buf1, "%s %s %s %s", pkt, sequencenb, reg, argv[1]);

            wc = send_packet(fd, buf1, strlen(buf1) + 1, 0, (struct sockaddr *)&dest_addr, addr_len);

            last_reg = time(NULL);

            tv.tv_sec = 10;
            tv.tv_usec = 0;
            FD_SET(fd, &fds);
        }

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(STDIN_FILENO, &fds);
        wc = select(FD_SETSIZE, &fds, NULL, NULL, &tv); 
        checkPerror(wc, "select2");


        if (FD_ISSET(fd, &fds)) 

        {
            rc = recvfrom(fd, output, BUFFSIZE - 1, 0, (struct sockaddr *)&clientaddr, &addr_len);
            checkPerror(rc, "recv");
            output[rc] = 0; 
            

            if (output[0] == 'A' && output[1] == 'C' && output[2] == 'K') 
            {
                if (output[6] == 'N' && output[7] == 'I' && output[8] == 'C' && output[9] == 'K')
                {
                    char *result[7];
                    int i = 0;
                    result[i] = strtok(output, " \n");

                    while (result[i] != NULL)
                    {
                        result[++i] = strtok(NULL, " \n");
                    }

                    char *to_nick = result[3];
                    portClient = strdup(result[7]);

                    
                    clientaddr.sin_family = AF_INET;
                    clientaddr.sin_port = atoi(portClient);
                    clientaddr.sin_addr = ip_addr;

                    registerC(to_nick, &clientaddr);

                    char msg_buffer[1500];
                    char *from = "FROM";
                    char *to = "TO";
                    char *msg = "MSG";
                    sprintf(msg_buffer, "%s %s %s %s %s %s %s %s", pkt, sequencenb, from, argv[1], to, to_nick, msg, msg_buffer1);
                    wc = send_packet(fd, msg_buffer, strlen(msg_buffer) + 1, 0, (struct sockaddr *)&clientaddr, addr_len);
                    checkPerror(wc, "send_msg");
                    
                }
                else if (output[12] == 'F' && output[13] == 'O' && output[14] == 'R' && output[15] == 'M' && output[16] == 'A' && output[17] == 'T')
                {
                    puts(output);
                }
                else if (output[12] == 'N' && output[13] == 'A' && output[14] == 'M' && output[15] == 'E')
                {
                    puts(output);
                }
                else if (output[0] == 'P' && output[1] == 'K' && output[2] == 'T')
                {
                    id_msg++;
                    
                }
                else
                {

                    puts(output);
                }
            }
            else
            {

                char *pkt = strtok(output, " ");
                char *number = strtok(NULL, " ");
                char *from = strtok(NULL, " ");
                char *from_nick_pack = strtok(NULL, " ");
                char *to = strtok(NULL, " ");
                char *to_nick_pack = strtok(NULL, " ");
                char *msg = strtok(NULL, " ");
                char *msg_text = strtok(NULL, "\n");
                

                registerM(from_nick_pack, msg_text, id_msg);

                
                char buf_answer[50];
                char *ack = "ACK";
                char *number_commando = number; 
                char *ok = "OK";
                char msg_stdout[BUFFSIZE + 4];

                if (strcmp(pkt, "PKT") != 0 && strcmp(from, "FROM") != 0 && strcmp(to, "TO") != 0 && strcmp(msg, "MSG") != 0)
                {
                    char buf_answerFormat[50];
                    sprintf(buf_answerFormat, "%s %s %s %s", ack, number_commando, "WRONG", "FORMAT");

                    wc = send_packet(fd, buf_answerFormat, strlen(buf_answerFormat) + 1, 0, (struct sockaddr *)&clientaddr, addr_len);
                    checkPerror(wc, "send_answer");

                    sprintf(msg_stdout, "%s : %s", from_nick_pack, msg_text);

                    puts(msg_stdout);
                }
                else if (strcmp(to_nick_pack, argv[1]) != 0)
                {
                    char buf_answerFormat[50];
                    sprintf(buf_answerFormat, "%s %s %s %s", ack, number_commando, "WRONG", "NAME");

                    wc = send_packet(fd, buf_answerFormat, strlen(buf_answerFormat) + 1, 0, (struct sockaddr *)&clientaddr, addr_len);
                    checkPerror(wc, "send_answer");
                }
                else
                {

                    sprintf(buf_answer, "%s %s %s", ack, number_commando, ok);

                    wc = send_packet(fd, buf_answer, strlen(buf_answer) + 1, 0, (struct sockaddr *)&clientaddr, addr_len);
                    checkPerror(wc, "send_answer");

                    sprintf(msg_stdout, "%s : %s", from_nick_pack, msg_text);
                    

                    puts(msg_stdout);
                }
            }
        }

        if (FD_ISSET(STDIN_FILENO, &fds))
        {
            get_string(buffer, BUFFSIZE);
            if (strcmp(buffer, "QUIT") == 0)
            {
                exit(1);
            }

            

            char *split1 = strtok(buffer, " ");
            char *text = strtok(NULL, "\n");

            tekst = strdup(text);
            msg_buffer1[0] = 0;
            strcpy(msg_buffer1, tekst);
            msg_buffer1[strlen(tekst)] = 0;

            char *to_nick = split1 + 1;

            struct clientCache *check = look_upC(to_nick, clientsCacheList);

            if (check != NULL)
            {

                wc = inet_pton(AF_INET, check->ip, &ip_addr_client);
                checkPerror(wc, "ip-client");
                if (!wc) 
                {
                    fprintf(stderr, "Invalid IP address: %s\n", check->ip);
                    return EXIT_FAILURE;
                }
                clientaddr.sin_family = AF_INET;
                clientaddr.sin_port = htons(check->port);
                clientaddr.sin_addr = ip_addr_client;

                char msg_buffer[BUFFSIZE];
                char *from = "FROM";
                char *to = "TO";
                char *msg = "MSG";
                sprintf(msg_buffer, "%s %s %s %s %s %s %s %s", pkt, sequencenb, from, argv[1], to, to_nick, msg, text);
                wc = send_packet(fd, msg_buffer, strlen(msg_buffer) + 1, 0, (struct sockaddr *)&clientaddr, addr_len);
                checkPerror(wc, "send_msg");
            }
            else
            {

                char buf2[50];
                char *lookup = "LOOKUP";

                sprintf(buf2, "%s %s %s %s", pkt, sequencenb, lookup, to_nick);
                wc = send_packet(fd, buf2, strlen(buf2) + 1, 0, (struct sockaddr *)&dest_addr, addr_len);
                checkPerror(wc, "send_lookup");
            }
        }
    }
    puts("Client closed");
    free_ClinkedList(clientsCacheList);
    free_MlinkedList(messageList);
    close(fd);
    return EXIT_SUCCESS;
}
