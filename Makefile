CC=gcc 
CFLAGS=-Wall 
mtserver: mtserver.o AcceptTCPConnection.o CreateTCPServerSocket.o DieWithError.o HandleTCPClient.o

clean:
	rm -f mtserver mtserver.o AcceptTCPConnection.o CreateTCPServerSocket.o DieWithError.o HandleTCPClient.o