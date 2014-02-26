#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */


#define BUFSIZE 150


//unsigned short cur_clients;			//current number of clients
//pthread_mutex_t lock;

struct client_info ; /* Forward declaration */

typedef struct client_info
{
	int sockfd;
	int failures;
	int total;
	int startIndex;
	int length;
	int mid_invalid;
	int exit;
	char cur_buff[BUFSIZE];

} client_info ;

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
int CreateTCPServerSocket(unsigned short port); /* Create TCP server socket */
int AcceptTCPConnection(int servSock);  /* Accept TCP connection request */
int CheckIncomming(client_info *ci); /* Checks the buffer for a full string from start to size */
int Uptime(client_info *ci); /*does the uptime call */
int Invalid(client_info *ci);
int Total(client_info *ci);
int Exit(client_info *ci);
int ServiceUptime(client_info *ci);
int ServiceLoad(client_info *ci);
int ServiceExit(client_info *ci);
int ServiceDigit(client_info *ci);
int ServiceDigit2(client_info *ci);
int Shutdown(client_info *ci);
int ServiceChar(client_info *ci) ;
