/**
 * SOCKET CODE AND STRUCTURE OF FILES IS FROM: Donahoo and Calvert from their book TCP/IP Sockets in C
 * (http://cs.baylor.edu/~donahoo/practical/CSockets)
 */
#include "mtserver.h"  /* TCP echo server includes */
#include <pthread.h>        /* for POSIX threads */

void *ThreadMain(void *arg); /* Main program of a thread */

/* Structure of arguments to pass to client thread */
struct ThreadArgs {
	int clntSock; /* Socket descriptor for client */
};

unsigned short cur_clients; //current number of clients
pthread_mutex_t lock;

int main(int argc, char *argv[]) {
	int servSock; /* Socket descriptor for server */
	int clntSock; /* Socket descriptor for client */
	unsigned short servPort; /* Server port */
	unsigned short max_clients; //number of selected clients
	pthread_t threadID; /* Thread ID from pthread_create() */
	struct ThreadArgs *threadArgs; /* Pointer to argument structure for thread */

	//============================ COMMAND LINE PARSING ================================
	if (argc != 3) /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <SERVER PORT>\n", argv[0]);
		exit(1);
	}

	max_clients = atoi(argv[1]); /*first arg: clients */
	servPort = atoi(argv[2]); /* First arg:  local port */
	cur_clients = 0;

	// ================= CREATING AND INITIALIZNG SERVER SOCK AND VARS ==================
	servSock = CreateTCPServerSocket(servPort);

	//deal with mutex:
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("\n mutex init failed\n");
		return 1;
	}

	// ======================== LISTEN FOR CLIENTS ====================================

	for (;;) /* run forever */
	{
		printf("Will shortly be accepting the connection from this client\n");
		clntSock = AcceptTCPConnection(servSock);

		//need to aquire the lock before accessing current number of clients
		pthread_mutex_lock(&lock);

		if (cur_clients < max_clients) {
			cur_clients++;
			printf("Max allowed clients: %d, Curent num clients connected: %d\n", max_clients, cur_clients);

			/* Create separate memory for client argument */
			if ((threadArgs = (struct ThreadArgs *) malloc(
					sizeof(struct ThreadArgs))) == NULL)
				DieWithError("malloc() failed");
			threadArgs -> clntSock = clntSock;

			/* Create client thread */
			if (pthread_create(&threadID, NULL, ThreadMain, (void *) threadArgs)
					!= 0) {
				DieWithError("pthread_create() failed");
			}

			printf("with thread %ld\n", (long int) threadID);
		}else{
			//accept and then close
			printf("Closing the socket on this client %d\n", clntSock);
			close(clntSock);
		}
		//todo: whats the deal with this!?
		pthread_mutex_unlock(&lock);
//		sleep(10);

	}

	pthread_mutex_destroy(&lock);

	/* NOT REACHED */
}

void *ThreadMain(void *threadArgs) {
	printf("Here thread main\n");
	int clntSock; /* Socket descriptor for client connection */

	/* Guarantees that thread resources are deallocated upon return */
	pthread_detach(pthread_self());

	/* Extract socket file descriptor from argument */
	clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;

	//now you want to read from the socket

	free(threadArgs); /* Deallocate memory for argument */
	HandleTCPClient(clntSock);

	return (NULL);
}
