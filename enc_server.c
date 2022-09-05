#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

/*
	Will print error outputs to stderr and close application.
*/
// Error function used for reporting issues
void error(const char *msg) {
//   perror(msg);
  fprintf(stderr, msg);
  exit(1);
} 
/*-------------------------------------------------*/
/*
	Intializes the address struct and sets values to accept connections.
*/
// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}
/*-------------------------------------------------*/
/*
	Sends string back to client
	primarily used for authentication & sending cipherText back
*/
int sendSocketMsg(int socketConn, const char* buff) {
	int charsRead;

	charsRead = send(socketConn, 
				buff, strlen(buff), 0); 
	if (charsRead < 0)
		error("ERROR writing to socket");
	return 1;
}
/*-------------------------------------------------*/
/*
	Iterates through plaintext encrypting each char with 
	the same index in the key+originalIndex
*/
void encryptMsg(char* plaintext, char* encryptKey, char* cipherText) {
	// printf("server pt3: %s\n", plaintext);
	for(int i=0; i < strlen(plaintext); i++) {
		if(plaintext[i] == 32) {
			strncat(cipherText, " ", 1);
		} else {
			// converting in range 0-25
			char x = (plaintext[i] + encryptKey[i]) %26;
			// convert into alphabets(ASCII)
			x += 'A';

			// printf("char: %c\n", x);
			strncat(cipherText, &x, 1);
		}
		
	}
}
//https://www.geeksforgeeks.org/vigenere-cipher/
/*-------------------------------------------------*/
/*
	Recieves the key and message which is passed to encryptMsg function.
*/
void getEncryptParts(int socketConn, struct sockaddr_in clientAddress) {
	int buffer_size = 80000, charsRead = 0, plaintextSize, encryptKeySize;
	char buffer[80000];
	char plaintext[80000];
	char encryptKey[80000];

	for(int i=0; i < 2; i++) {
		// iterate until message is recieved from client.
		memset(buffer, '\0', 256);
		while(1) {
			charsRead = recv(socketConn, buffer, 255, 0);
			if (charsRead < 0)
				error("ERROR reading from socket");
			else
				break;
		}
		
		switch(i) {
			case 0: strcpy(plaintext, buffer); break;
			case 1: strcpy(encryptKey, buffer); break;
			default: break;
		}
	}
	// printf("server pt2: %s\n", plaintext);
	// printf("encryptKey: %s\n", encryptKey);

	char encryptedMessage[256];
	encryptMsg(plaintext, encryptKey, encryptedMessage);
	// printf("encryptMessage: %s\n", encryptedMessage);

	sendSocketMsg(socketConn, encryptedMessage);
	
}
/*-------------------------------------------------*/
/*
	Sends and recieves key value to authenticate client with server.
*/
int authSocket(int socketConn, struct sockaddr_in clientAddress) {
	int charsRead;
	char buffer[256];
	char clientToken[] = "client1001";

	printf("SERVER: Connected to client running at host %d port %d\n", 
					ntohs(clientAddress.sin_addr.s_addr),
					ntohs(clientAddress.sin_port));


	// Get the message from the client and display it
	memset(buffer, '\0', 256);
	// read client msg from socket
	charsRead = recv(socketConn, buffer, 255, 0); 
	if (charsRead < 0)
		error("ERROR reading from socket");
	
	if( strcmp(buffer, clientToken) == 0) {
		// printf("SERVER: client authenticated \n");			//uncomment for debugging
		sendSocketMsg(socketConn, clientToken);
		//get plaintext & key to encrypt message
		getEncryptParts(socketConn, clientAddress);
	} else {
		sendSocketMsg(socketConn, "Invalid client; not authenticated");
	}

	// Close the connection socket for this client
	close(socketConn);
	//close the forked() proc
	exit(0);
	return 1;
}
/*-------------------------------------------------*/
/*
	Checks if a connection is dead to remove from allSockets array
*/
#define NUM_SOCKETS 5
void removeDeadSockets(int *allSockets, int *socketPID) {
	pid_t wpid;
	int childStatus;

	for(int i=0; i < (NUM_SOCKETS); i++) {
		wpid = waitpid(socketPID[i], &childStatus, WNOHANG);

		if( (WIFSIGNALED(childStatus)) || (WIFEXITED(childStatus)) )
			allSockets[i] = -1;
	}
}
/*-------------------------------------------------*/
/*
	This funciton accepts up to 5 socket connection
	Will fork a new proc for every connection 
	& call authSocket to authenticate once accepted
*/
void acceptSocketConn(int listenSocket) {
	// int *allSockets = malloc((sizeof(int))*(NUM_SOCKETS));
	int allSockets[NUM_SOCKETS] = {-1, -1, -1, -1, -1}, socketPID[NUM_SOCKETS] = {-1, -1, -1, -1, -1};
	struct sockaddr_in clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress);

	while(1) {
		// Accept the connection request which creates a connection socket
		removeDeadSockets(allSockets, socketPID);
		for(int i=0; i < (NUM_SOCKETS); i++) {
			// printf("socket[%d]: %d\n", i, allSockets[i]);
			if(allSockets[i] > 0)
				continue;

			allSockets[i] = accept(listenSocket, 
					(struct sockaddr *)&clientAddress, 
					&sizeOfClientInfo);

			if (allSockets[i]  < 0)
				error("ERROR on accept");

			socketPID[i] = fork();
			switch (socketPID[i]) {
				case -1:
					//when fork() fails
					error("fork() failed!");
					break;
				case 0:
					//fork() proc here
					authSocket(allSockets[i], clientAddress);
					break;
				default:
					//parent executes here
					break;
			}
		}
  }
}

/*-------------------------------------------------*/
int main(int argc, char *argv[]){
  struct sockaddr_in serverAddress;
//   socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  }
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 

  //createSocketConn handles all accepted connections
  acceptSocketConn(listenSocket);
  
  close(listenSocket); 
  return 0;
}
