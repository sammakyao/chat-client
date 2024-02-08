#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_BUFFER 1024

void chatloop(char *name, int socketFd);
void buildMessage(char *result, char *name, char *msg);
void setupAndConnect(struct sockaddr_in *serverAddr, struct hostent *host, int socketFd, long port);
void setNonBlock(int fd);
void interruptHandler(int sig);
void introMessage(char *name, int socketFd);

static int socketFd;

int main(int argc, char *argv[])
{
    char *name;
    struct sockaddr_in serverAddr;
    struct hostent *host;
    long port;

    if(argc != 4)
    {
        fprintf(stderr, "./client [host] [port] [username]\n");
        exit(1);
    }
    name = argv[3];
    if((host = gethostbyname(argv[1])) == NULL)
    {
        fprintf(stderr, "Couldn't get host name\n");
        exit(1);
    }
    port = strtol(argv[2], NULL, 0);
    if((socketFd = socket(AF_INET, SOCK_STREAM, 0))== -1)
    {
        fprintf(stderr, "Couldn't create socket\n");
        exit(1);
    }

    setupAndConnect(&serverAddr, host, socketFd, port);
    setNonBlock(socketFd);
    setNonBlock(0);

    //Set a handler for the interrupt signal
    signal(SIGINT, interruptHandler);

    introMessage(name, socketFd);
    chatloop(name, socketFd);
}

//Main loop that takes in chat input and display output
void chatloop(char *name, int socketFd)
{
    fd_set clientFds;
    char chatMsg[MAX_BUFFER];
    char chatBuffer[MAX_BUFFER], msgBuffer[MAX_BUFFER];

    while(1)
    {
        // Here it resets the fd set each time since select() modifies it
        FD_ZERO(&clientFds);
        FD_SET(socketFd, &clientFds);
        FD_SET(0, &clientFds);
        if(select(FD_SETSIZE, &clientFds, NULL, NULL, NULL) != -1) // here it waits for an available fd
        {
            for(int fd = 0; fd < FD_SETSIZE; fd++)
            {
                if(FD_ISSET(fd, &clientFds))
                {
                    if(fd == socketFd) // here it receives data from server
                    {
                        int numBytesRead = read(socketFd, msgBuffer, MAX_BUFFER - 1);
                        msgBuffer[numBytesRead] = '\0';
                        printf("%s", msgBuffer);
                        memset(&msgBuffer, 0, sizeof(msgBuffer));
                    }
                    else if(fd == 0) // here it reads from keyboard (stdin) and send to server
                    {
                        fgets(chatBuffer, MAX_BUFFER - 1, stdin);
                        if(strcmp(chatBuffer, "/exit\n") == 0)
                            interruptHandler(-1); //Reuse the interruptHandler function to disconnect the client
                        else
                        {
                            if(write(socketFd, "3", MAX_BUFFER - 1) == -1)
                                perror("write failed: ");
                            else{
                                buildMessage(chatMsg, name, chatBuffer);
                                if(write(socketFd, chatMsg, MAX_BUFFER - 1) == -1) perror("write failed: ");
                                //printf("%s", chatMsg);
                                memset(&chatBuffer, 0, sizeof(chatBuffer));
                            }

                        }
                    }
                }
            }
        }
    }
}

void introMessage(char *name, int socketFd){
    if(write(socketFd, "0", MAX_BUFFER - 1) == -1)
        perror("write failed: ");

    if(write(socketFd, name, MAX_BUFFER - 1) == -1)
        perror("write failed: ");
    printf("Welcome to the server %s.\n", name);
}

// Below it concatenates the name with the message and puts it into result
void buildMessage(char *result, char *name, char *msg)
{
    memset(result, 0, MAX_BUFFER);
    strcpy(result, name);
    strcat(result, ": ");
    strcat(result, msg);
}

// Here it sets up the socket and connects
void setupAndConnect(struct sockaddr_in *serverAddr, struct hostent *host, int socketFd, long port)
{
    memset(serverAddr, 0, sizeof(serverAddr));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    serverAddr->sin_port = htons(port);
    if(connect(socketFd, (struct sockaddr *) serverAddr, sizeof(struct sockaddr)) < 0)
    {
        perror("Couldn't connect to server");
        exit(1);
    }
}

// Below it sets the fd to nonblocking
void setNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        perror("fcntl failed");

    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

// It notifies the server when the client exits by sending "/exit"
void interruptHandler(int sig_unused)
{
    if(write(socketFd, "5", MAX_BUFFER - 1) == -1)
        perror("write failed: ");
    char *msgBuffer = malloc(MAX_BUFFER-1);
    int numBytesRead = read(socketFd, msgBuffer, MAX_BUFFER - 1);
  
        printf("The connection is closing.\n");
        printf("Bye bye!\n");
        close(socketFd);
        exit(1);
    //}
}
